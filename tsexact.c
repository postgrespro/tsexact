#include "postgres.h"

#include "c.h"
#include "fmgr.h"
#include "tsearch/ts_type.h"
#include "tsearch/ts_utils.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(ts_exact_match);
Datum		ts_exact_match(PG_FUNCTION_ARGS);

typedef struct
{
	WordEntry  *arrb;
	WordEntry  *arre;
	char	   *values;
	char	   *operand;
} CHKVAL;

/*
 * is there value 'val' in array or not ?
 */
static WordEntry *
checkcondition_str(CHKVAL *chkval, QueryOperand *val)
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
		difference = tsCompareString(chkval->operand + val->distance, val->length,
						   chkval->values + StopMiddle->pos, StopMiddle->len,
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
	QueryOperand *operand;
	WordEntryPos *pos;
	int len;
} OperandInfo;

#define PREALLOC_SIZE 128

/*
 * Checks if tsvector exactly matches tsquery
 */
Datum
ts_exact_match(PG_FUNCTION_ARGS)
{
	TSVector	val = PG_GETARG_TSVECTOR(0);
	TSQuery		query = PG_GETARG_TSQUERY(1);
	QueryItem  *items = GETQUERY(query);
	CHKVAL		chkval;
	int 		i, j;
	OperandInfo	opInfoPrealloc[PREALLOC_SIZE], *opInfo;

	if (query->size > PREALLOC_SIZE)
		opInfo = (OperandInfo *)palloc(sizeof(OperandInfo) * query->size);
	else
		opInfo = opInfoPrealloc;

	chkval.arrb = ARRPTR(val);
	chkval.arre = chkval.arrb + val->size;
	chkval.values = STRPTR(val);
	chkval.operand = GETOPERAND(query);

	j = 0;
	for (i = query->size - 1; i >=0 ; i--)
	{
		if (items[i].type == QI_VAL)
		{
			WordEntry *we;
			we = checkcondition_str(&chkval, (QueryOperand *)&items[i]);
			if (!we)
			{
				if (opInfo != opInfoPrealloc)
					pfree(opInfo);
				PG_RETURN_BOOL(false);
			}
			opInfo[j].operand = (QueryOperand *)&items[i];
			opInfo[j].pos = POSDATAPTR(val, we);
			opInfo[j].len = POSDATALEN(val, we);
			j++;
		}
	}

	while (opInfo[0].len > 0)
	{
		uint16 pos = *opInfo[0].pos;
		for (i = 0; i < j; i++)
		{
			while (opInfo[i].len > 0 && *opInfo[i].pos < pos + i)
			{
				opInfo[i].pos++;
				opInfo[i].len--;
			}
			if (opInfo[i].len <= 0)
			{
				if (opInfo != opInfoPrealloc)
					pfree(opInfo);
				PG_RETURN_BOOL(false);
			}
			if (*opInfo[i].pos > pos + i)
				break;
		}
		if (i == j)
		{
			if (opInfo != opInfoPrealloc)
				pfree(opInfo);
			PG_RETURN_BOOL(true);
		}
		opInfo[0].pos++;
		opInfo[0].len--;
	}

	if (opInfo != opInfoPrealloc)
		pfree(opInfo);
	PG_RETURN_BOOL(false);
}
