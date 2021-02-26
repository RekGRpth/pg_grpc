-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_grpc" to load this file. \quit

CREATE OR REPLACE FUNCTION grpc(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_grpc' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION grpc(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_grpc' LANGUAGE 'c';
