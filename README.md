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

TSExact offers following functions.

|          Function                                                 | Return type |                      Description                           |
| ----------------------------------------------------------------- | ----------- | ---------------------------------------------------------- |
| ts_exact_match(document tsvector, flagment tsvector)              | bool        | Check if given fragment is present in document             |
| ts_exact_match(document tsvector, flagment tsvector, weight text) | bool        | Check if given fragment is present in document with weight |
| ts_squeeze(document tsvector)                                     | tsvector    | Remove empty positions from document                       |
| setweight(query tsquery, weight text)                             | tsquery     | Set weight for each lexeme in tsquery                      |
| poslen(documents tsvector)                                        | integer     | Return total number of positions in document               |
