CREATE EXTENSION tsexact;

SELECT ts_exact_match('a:2 b:3 c:5'::tsvector, 'a:1 b:2 c:4'::tsvector);
SELECT ts_exact_match('a:2 b:3 c:4'::tsvector, 'a:1 b:2 c:4'::tsvector);
SELECT ts_exact_match('a:2 b:3 c:4'::tsvector, 'a:1 b:2 c:3'::tsvector);
SELECT ts_exact_match('a:1,2,6,7,8 b:3,4,5,9,10'::tsvector, 'a:1,2,3 b:4,5'::tsvector);
SELECT ts_exact_match('a:1,2,6,7 b:3,4,5,8,9'::tsvector, 'a:1,2,3 b:4,5'::tsvector);

SELECT ts_exact_match('a:1 b:2'::tsvector, 'a:1 b:2 c:2'::tsvector);
SELECT ts_exact_match('a:1 c:2'::tsvector, 'a:1 b:2 c:2'::tsvector);
SELECT ts_exact_match('a:1 d:2'::tsvector, 'a:1 b:2 c:2'::tsvector);
SELECT ts_exact_match('a:1 b:2 c:2'::tsvector, 'a:1 b:2 c:2'::tsvector);

SELECT ts_squeeze('a:1,6 b:2,9 c:4'::tsvector);
SELECT ts_squeeze('a:2,10 b:5,6 c:8 d:12'::tsvector);

SELECT setweight('a & b'::tsquery, 'A');
SELECT setweight('a:A & (b:B | c:C)'::tsquery, 'CD');
SELECT setweight('a:B | b:AD'::tsquery, '');
