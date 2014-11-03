/* tsexact/tsexact--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION tsexact" to load this file. \quit

--
--  PostgreSQL code for TSEXACT.
--

CREATE FUNCTION ts_exact_match(tsvector, tsvector)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ts_exact_match(tsvector, tsvector, text)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ts_squeeze(tsvector)
RETURNS tsvector
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;
