-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_grpc" to load this file. \quit

CREATE OR REPLACE FUNCTION grpc_insecure_channel_create(target text) RETURNS boolean AS 'MODULE_PATHNAME', 'pg_grpc_insecure_channel_create' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION grpc_secure_channel_create(target text) RETURNS boolean AS 'MODULE_PATHNAME', 'pg_grpc_secure_channel_create' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION grpc_channel_create_call(method text) RETURNS boolean AS 'MODULE_PATHNAME', 'pg_grpc_channel_create_call' LANGUAGE 'c';
