#include "postgres.h"

#include "c.h"
#include "fmgr.h"
#include "tsearch/ts_type.h"
#include "tsearch/ts_utils.h"
#include "utils/memutils.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(ts_exact_match);
Datum		ts_exact_match(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(ts_squeeze);
Datum		ts_squeeze(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(setweight_tsquery);
Datum		setweight_tsquery(PG_FUNCTION_ARGS);

typedef struct
{
	WordEntry  *arrb;
	WordEntry  *arre;
	char	   *arrvalues;
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

static uint8
getWeightMask(text *weight)
{
	uint8	weightMask = 0;
	char   *w, *we;

	weightMask = 0;
	w = VARDATA_ANY(weight);
	we = w + VARSIZE_ANY_EXHDR(weight);

	while (w < we)
	{
		switch (*w)
		{
		    case 'A':
		    case 'a':
		    	weightMask |= (1 << 3);
		        break;
		    case 'B':
		    case 'b':
		    	weightMask |= (1 << 2);
		        break;
		    case 'C':
		    case 'c':
		    	weightMask |= (1 << 1);
		        break;
		    case 'D':
		    case 'd':
		    	weightMask |= (1 << 0);
		        break;
		    default:
		        /* internal error */
		        elog(ERROR, "unrecognized weight: %d", *w);
		}
		w++;
	}
	return weightMask;
}

static void
invalidateCache(TSVector query)
{
	int				len, i, j, k;
	OperandInfo	   *opInfo;
	WordEntry	   *we;

	if (cachedQuery && VARSIZE_ANY(query) == VARSIZE_ANY(cachedQuery) &&
			!memcmp(query, cachedQuery, VARSIZE_ANY(query)))
		return;

	if (cachedQuery)
		free(cachedQuery);
	if (cachedOpInfo)
		free(cachedOpInfo);

	cachedQuery = (TSVector)malloc(VARSIZE_ANY(query));
	memcpy(cachedQuery, query, VARSIZE_ANY(query));

	we = ARRPTR(cachedQuery);

	cachedOpInfoLen = 0;
	for (i = 0; i < cachedQuery->size; i++)
		cachedOpInfoLen += POSDATALEN(cachedQuery, &we[i]);

	k = 0;
	opInfo = (OperandInfo *)malloc(sizeof(OperandInfo) * cachedOpInfoLen);
	for (i = 0; i < cachedQuery->size; i++)
	{
		WordEntryPos *pos = POSDATAPTR(cachedQuery, &we[i]);
		len = POSDATALEN(cachedQuery, &we[i]);
		for (j = 0; j < len; j++)
		{
			opInfo[k].we = &we[i];
			opInfo[k].index = WEP_GETPOS(*pos);
			k++; pos++;
		}
	}
	qsort(opInfo, cachedOpInfoLen, sizeof(OperandInfo), operandInfoCmp);
	cachedOpInfo = opInfo;
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
	int 		i;
	uint8		weightMask;
	OperandInfo	*opInfo;
	bool		notFound = false;

	if (PG_NARGS() >= 3)
		weightMask = getWeightMask(PG_GETARG_TEXT_PP(2));
	else
		weightMask = 0xF;

	invalidateCache(query);

	chkval.arrb = ARRPTR(val);
	chkval.arre = chkval.arrb + val->size;
	chkval.arrvalues = STRPTR(val);
	chkval.queryvalues = STRPTR(cachedQuery);

	opInfo = cachedOpInfo;

	for (i = 0; i < cachedOpInfoLen; i++)
	{
		WordEntry *we;
		we = checkcondition_str(&chkval, opInfo[i].we);
		if (!we)
		{
			if (i == 0 || opInfo[i].index != opInfo[i - 1].index)
				notFound = true;
			if (notFound && (i == cachedOpInfoLen - 1 || opInfo[i].index != opInfo[i + 1].index))
				PG_RETURN_BOOL(false);
			opInfo[i].pos = NULL;
			opInfo[i].len = 0;
		}
		else
		{
			notFound = false;
			opInfo[i].pos = POSDATAPTR(val, we);
			opInfo[i].len = POSDATALEN(val, we);
		}
	}

	while (opInfo[0].len > 0)
	{
		bool finished = false;
		int pos = (int)(*opInfo[0].pos) - opInfo[0].index;

		if (!(weightMask & (1 << WEP_GETWEIGHT(pos))))
		{
			opInfo[0].pos++;
			opInfo[0].len--;
			continue;
		}

		for (i = 0; i < cachedOpInfoLen; i++)
		{
			while (opInfo[i].len > 0 && (int)(*opInfo[i].pos) < pos + opInfo[i].index)
			{
				opInfo[i].pos++;
				opInfo[i].len--;
			}
			if (opInfo[i].len <= 0)
			{
				if (i == 0 || opInfo[i].index != opInfo[i - 1].index)
					finished = true;
				if (finished && (i == cachedOpInfoLen - 1 || opInfo[i].index != opInfo[i + 1].index))
					PG_RETURN_BOOL(false);
				continue;
			}
			else
			{
				finished = false;
			}
			if ((int)*opInfo[i].pos > pos + opInfo[i].index)
			{
				if (i < cachedOpInfoLen - 1 && opInfo[i].index == opInfo[i + 1].index)
					continue;
				else
					break;
			}
			else
			{
				while (i < cachedOpInfoLen - 1 && opInfo[i].index == opInfo[i + 1].index)
					i++;
			}
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

static int
cmpPos(const void *a1, const void *a2)
{
	const WordEntryPos **pos1, **pos2;
	uint16 w1, w2, p1, p2;

	pos1 = (const WordEntryPos **)a1;
	pos2 = (const WordEntryPos **)a2;

	w1 = WEP_GETWEIGHT(**pos1);
	w2 = WEP_GETWEIGHT(**pos2);

	if (w1 < w2)
		return -1;
	else if (w1 > w2)
		return 1;

	p1 = WEP_GETPOS(**pos1);
	p2 = WEP_GETPOS(**pos2);

	if (p1 < p2)
		return -1;
	else if (p1 == p2)
		return 0;
	else
		return 1;
}

Datum
ts_squeeze(PG_FUNCTION_ARGS)
{
	TSVector		val = PG_GETARG_TSVECTOR_COPY(0);
	WordEntry	   *we;
	WordEntryPos  **pos;
	int				i, j, k, len;
	uint16			w, p, prev_p;

	we = ARRPTR(val);
	len = 0;
	for (i = 0; i < val->size; i++)
		len += POSDATALEN(val, &we[i]);

	pos = (WordEntryPos **)palloc(sizeof(WordEntryPos *) * len);
	k = 0;
	for (i = 0; i < val->size; i++)
	{
		for (j = 0; j < POSDATALEN(val, &we[i]); j++)
		{
			pos[k] = POSDATAPTR(val, &we[i]) + j;
			k++;
		}
	}

	qsort(pos, len, sizeof(WordEntryPos *), cmpPos);

	w = 4; p = 1, prev_p = 1;
	for (i = 0; i < len; i++)
	{
		if (WEP_GETWEIGHT(*pos[i]) != w)
		{
			p = 1;
			w = WEP_GETWEIGHT(*pos[i]);
			prev_p = WEP_GETPOS(*pos[i]);
		}
		else
		{
			if (WEP_GETPOS(*pos[i]) > prev_p)
			{
				p++;
				prev_p = WEP_GETPOS(*pos[i]);
			}
		}
		WEP_SETPOS(*pos[i], p);
	}

	PG_RETURN_TSVECTOR(val);
}

Datum
setweight_tsquery(PG_FUNCTION_ARGS)
{
	TSQuery		query = PG_GETARG_TSQUERY_COPY(0);
	uint8		weightMask;
	QueryItem  *items;
	int			i;

	items = GETQUERY(query);
	weightMask = getWeightMask(PG_GETARG_TEXT_PP(1));
	for (i = 0; i < query->size; i++)
	{
		if (items[i].type == QI_VAL)
		{
			items[i].qoperand.weight = weightMask;
		}
	}
	PG_RETURN_TSQUERY(query);
}
