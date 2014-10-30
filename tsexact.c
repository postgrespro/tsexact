#include "postgres.h"

#include "c.h"
#include "fmgr.h"
#include "tsearch/ts_type.h"
#include "tsearch/ts_utils.h"
#include "utils/memutils.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(ts_exact_match);
Datum		ts_exact_match(PG_FUNCTION_ARGS);

typedef struct
{
	WordEntry  *arrb;
	WordEntry  *arre;
	char	   *arrvalues;
	WordEntry  *queryb;
	WordEntry  *querye;
	char	   *queryvalues;
} CHKVAL;

/*
 * is there value 'val' in array or not ?
 */
static WordEntry *
checkcondition_str(CHKVAL *chkval, WordEntry *val)
{
	WordEntry  *StopLow = chkval->arrb;
	WordEntry  *StopHigh = chkval->arre;
	WordEntry  *StopMiddle = StopHigh;
	int			difference = -1;
	bool		res = false;

	/* Loop invariant: StopLow <= val < StopHigh */
	while (StopLow < StopHigh)
	{
		StopMiddle = StopLow + (StopHigh - StopLow) / 2;
		difference = tsCompareString(chkval->queryvalues + val->pos, val->len,
						   chkval->arrvalues + StopMiddle->pos, StopMiddle->len,
									 false);

		if (difference == 0)
		{
			res = true;
			break;
		}
		else if (difference > 0)
			StopLow = StopMiddle + 1;
		else
			StopHigh = StopMiddle;
	}

	if (res)
		return StopMiddle;
	else
		return NULL;
}

typedef struct
{
	WordEntry *we;
	WordEntryPos *pos;
	int len, index;
} OperandInfo;

#define PREALLOC_SIZE 128


TSVector cachedQuery = NULL;
OperandInfo *cachedOpInfo = NULL;
int	cachedOpInfoLen = 0;

static int
operandInfoCmp(const void *a1, const void *a2)
{
	const OperandInfo *o1 = (const OperandInfo *)a1;
	const OperandInfo *o2 = (const OperandInfo *)a2;

	if (o1->index < o2->index)
		return -1;
	else if (o1->index == o2->index)
		return 0;
	else
		return 1;
}

/*
 * Checks if tsvector exactly matches tsquery
 */
Datum
ts_exact_match(PG_FUNCTION_ARGS)
{
	TSVector	val = PG_GETARG_TSVECTOR(0);
	TSVector	query = PG_GETARG_TSVECTOR(1);
	CHKVAL		chkval;
	int 		i, j, k;
	OperandInfo	*opInfo;

	chkval.arrb = ARRPTR(val);
	chkval.arre = chkval.arrb + val->size;
	chkval.arrvalues = STRPTR(val);

	if (!cachedQuery || VARSIZE_ANY(query) != VARSIZE_ANY(cachedQuery) ||
			memcmp(query, cachedQuery, VARSIZE_ANY(query)))
	{
		int len;

		if (cachedQuery)
			free(cachedQuery);
		if (cachedOpInfo)
			free(cachedOpInfo);
		cachedQuery = (TSVector)malloc(VARSIZE_ANY(query));
		memcpy(cachedQuery, query, VARSIZE_ANY(query));

		chkval.queryb = ARRPTR(cachedQuery);
		chkval.querye = chkval.queryb + cachedQuery->size;
		chkval.queryvalues = STRPTR(cachedQuery);

		cachedOpInfoLen = 0;
		for (i = 0; i < cachedQuery->size; i++)
			cachedOpInfoLen += POSDATALEN(cachedQuery, &chkval.queryb[i]);

		k = 0;
		opInfo = (OperandInfo *)malloc(sizeof(OperandInfo) * cachedOpInfoLen);
		for (i = 0; i < cachedQuery->size; i++)
		{
			WordEntryPos *pos = POSDATAPTR(cachedQuery, &chkval.queryb[i]);
			len = POSDATALEN(cachedQuery, &chkval.queryb[i]);
			for (j = 0; j < len; j++)
			{
				opInfo[k].we = &chkval.queryb[i];
				opInfo[k].index = WEP_GETPOS(*pos);
				k++; pos++;
			}
		}
		qsort(opInfo, cachedOpInfoLen, sizeof(OperandInfo), operandInfoCmp);
		cachedOpInfo = opInfo;
	}
	else
	{
		opInfo = cachedOpInfo;
		chkval.queryb = ARRPTR(cachedQuery);
		chkval.querye = chkval.queryb + cachedQuery->size;
		chkval.queryvalues = STRPTR(cachedQuery);
	}

	for (i = 0; i < cachedOpInfoLen; i++)
	{
		WordEntry *we;
		we = checkcondition_str(&chkval, opInfo[i].we);
		if (!we)
		{
			PG_RETURN_BOOL(false);
		}
		opInfo[i].pos = POSDATAPTR(val, we);
		opInfo[i].len = POSDATALEN(val, we);
	}

	while (opInfo[0].len > 0)
	{
		int pos = (int)(*opInfo[0].pos) - opInfo[0].index;
		for (i = 0; i < cachedOpInfoLen; i++)
		{
			while (opInfo[i].len > 0 && (int)(*opInfo[i].pos) < pos + opInfo[i].index)
			{
				opInfo[i].pos++;
				opInfo[i].len--;
			}
			if (opInfo[i].len <= 0)
			{
				PG_RETURN_BOOL(false);
			}
			if ((int)*opInfo[i].pos > pos + opInfo[i].index)
				break;
		}
		if (i == cachedOpInfoLen)
		{
			PG_RETURN_BOOL(true);
		}
		opInfo[0].pos++;
		opInfo[0].len--;
	}

	PG_RETURN_BOOL(false);
}
