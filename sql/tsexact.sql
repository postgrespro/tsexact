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
SELECT ts_exact_match(''::tsvector, ''::tsvector);
SELECT ts_exact_match('a:1'::tsvector, ''::tsvector);
SELECT ts_exact_match(''::tsvector, 'a:1'::tsvector);

SELECT ts_exact_match('a:2A b:3B c:5C'::tsvector, 'a:1 b:2 c:4'::tsvector, 'ABC');
SELECT ts_exact_match('a:2A b:3B c:5'::tsvector, 'a:1 b:2 c:4'::tsvector, 'ABC');
SELECT ts_exact_match('a:2 b:3 c:5'::tsvector, 'a:1A b:2B c:4C'::tsvector, 'D');
SELECT ts_exact_match('a:1A,4C b:2B,5 c:3A,6C'::tsvector, 'a:1A b:2B c:3C'::tsvector, 'CD');
SELECT ts_exact_match('a:1A,4C b:2C,5 c:3A,6B'::tsvector, 'a:1A b:2B c:3C'::tsvector, 'CD');

SELECT ts_squeeze('a:1,6 b:2,9 c:4'::tsvector);
SELECT ts_squeeze('a:2,10 b:5,6 c:8 d:12'::tsvector);
SELECT ts_squeeze('a:2A,10 b:5B,6C c:8 d:12A'::tsvector);

SELECT setweight('a & b'::tsquery, 'A');
SELECT setweight('a:A & (b:B | c:C)'::tsquery, 'CD');
SELECT setweight('a:B | b:AD'::tsquery, '');

SELECT poslen('a:1 b:2'::tsvector);
SELECT poslen('a:2A,10 b:5B,6C c:8 d:12A'::tsvector);
SELECT poslen('a:1,2,6,7,8 b:3,4,5,9,10'::tsvector);
