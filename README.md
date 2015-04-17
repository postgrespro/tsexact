TSExact – PostgreSQL fulltext search addon
==========================================

Introduction
------------

TSExact – is a PostgreSQL extension with various helper function for fulltext
search. 


Authors
-------

 * Alexander Korotkov <aekorotkov@gmail.com>, Postgres Professional, Moscow, Russia

Availability
------------

TSExact is released as an extension and not available in default PostgreSQL
installation. It is available from
[github](https://github.com/akorotkov/tsexact)
under the same license as
[PostgreSQL](http://www.postgresql.org/about/licence/)
and supports PostgreSQL 9.0+.

Installation
------------

Before build and install TSExact you should ensure following:
    
 * PostgreSQL version is 9.0 or higher.
 * You have development package of PostgreSQL installed or you built
   PostgreSQL from source.
 * Your PATH variable is configured so that pg\_config command available.
    
Typical installation procedure may look like this:
    
    $ git clone https://github.com/akorotkov/tsexact.git
    $ cd tsexact
    $ make USE_PGXS=1
    $ sudo make USE_PGXS=1 install
    $ make USE_PGXS=1 installcheck
    $ psql DB -c "CREATE EXTENSION tsexact;"

Usage
-----

TSExact offers various helper functions which are listed in the table below. In particular these functions could be used for simple fulltext search.

|          Function                                                 | Return type |                      Description                           |
| ----------------------------------------------------------------- | ----------- | ---------------------------------------------------------- |
| ts_exact_match(document tsvector, flagment tsvector)              | bool        | Check if given fragment is present in document             |
| ts_exact_match(document tsvector, flagment tsvector, weight text) | bool        | Check if given fragment is present in document with weight |
| ts_squeeze(document tsvector)                                     | tsvector    | Remove empty positions from document                       |
| setweight(query tsquery, weight text)                             | tsquery     | Assign weight for each lexeme in tsquery                   |
| poslen(documents tsvector)                                        | integer     | Return total number of positions in document               |

`ts_exact_match(tsvector, tsvector)` function checks if given fragment appears in given document at some offset.

    SELECT ts_exact_match('a:2 b:3 c:4'::tsvector, 'a:1 b:2 c:4'::tsvector);
     ts_exact_match 
    ----------------
     f
    (1 row)

    SELECT ts_exact_match('a:2 b:3 c:5'::tsvector, 'a:1 b:2 c:4'::tsvector);
     ts_exact_match 
    ----------------
     t
    (1 row)

`ts_exact_match(tsvector, tsvector)` ignores lexemes weights. `ts_exact_match(tsvector, tsvector, text)` only finds fragments in given weight of document. Weights of fragment are always ignored.

    SELECT ts_exact_match('a:2 b:3 c:4'::tsvector, 'a:1 b:2 c:4'::tsvector, 'ABC');
     ts_exact_match 
    ----------------
     f
    (1 row)

    SELECT ts_exact_match('a:2A b:3B c:5C'::tsvector, 'a:1 b:2 c:4'::tsvector, 'ABC');
     ts_exact_match 
    ----------------
     t
    (1 row)

Since tsvectors could contain gaps in position numbering it's suitable to remove gaps using `ts_squeeze(tsvector)`.

    SELECT ts_squeeze('a:1,6 b:2,9 c:4'::tsvector);
          ts_squeeze       
    -----------------------
     'a':1,4 'b':2,5 'c':3
    (1 row)

Fulltext search indexes doesn't support `ts_exact_match()` functions. Thus it's useful to combine `ts_exact_match()` with `tsvector @@ tsquery` expression in order to use indexed search.

`setweight(tsquery, text)` assigns given weight to each lexeme of tsquery.

    SELECT setweight('a:A & (b:B | c:C)'::tsquery, 'CD');
              setweight           
    ------------------------------
     'a':CD & ( 'b':CD | 'c':CD )
    (1 row)

`poslen(tsvector)` returns total number of lexeme positions in tsvector.

    SELECT poslen('a:1,2,6,7,8 b:3,4,5,9,10'::tsvector);
     poslen 
    --------
         10
    (1 row)
