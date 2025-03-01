/*
 * Copyright OpenSearch Contributors
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef IT_ODBC_HELPER_H
#define IT_ODBC_HELPER_H

#ifdef WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

#include <iostream>
#include <vector>

#include "unit_test_helper.h"

// SQLSTATEs
#define SQLSTATE_STRING_DATA_RIGHT_TRUNCATED (SQLWCHAR*)L"01004"
#define SQLSTATE_INVALID_DESCRIPTOR_INDEX (SQLWCHAR*)L"07009"
#define SQLSTATE_GENERAL_ERROR (SQLWCHAR*)L"HY000"
#define SQLSTATE_INVALID_DESCRIPTOR_FIELD_IDENTIFIER (SQLWCHAR*)L"HY091"

#define IT_SIZEOF(x) (NULL == (x) ? 0 : (sizeof((x)) / sizeof((x)[0])))

std::vector< std::pair< std::wstring, std::wstring > > conn_str_pair = {
    {L"Driver", L"{OpenSearch ODBC}"},
    {L"host", (use_ssl ? L"https://localhost" : L"localhost")},
    {L"port", L"9200"},
    {L"user", L"admin"},
    {L"password", L"admin"},
    {L"auth", L"BASIC"},
    {L"useSSL", (use_ssl ? L"1" : L"0")},
    {L"hostnameVerification", L"0"},
    {L"logLevel", L"0"},
    {L"logOutput", L"C:\\"},
    {L"responseTimeout", L"10"},
    {L"fetchSize", L"0"}};

std::wstring conn_string = []() {
    std::wstring temp;
    for (auto it : conn_str_pair)
        temp += it.first + L"=" + it.second + L";";
    return temp;
}();

void AllocConnection(SQLHENV* db_environment, SQLHDBC* db_connection,
                     bool throw_on_error, bool log_diag);
void ITDriverConnect(SQLTCHAR* connection_string, SQLHENV* db_environment,
                     SQLHDBC* db_connection, bool throw_on_error,
                     bool log_diag);
void AllocStatement(SQLTCHAR* connection_string, SQLHENV* db_environment,
                    SQLHDBC* db_connection, SQLHSTMT* h_statement,
                    bool throw_on_error, bool log_diag);
void LogAnyDiagnostics(SQLSMALLINT handle_type, SQLHANDLE handle, SQLRETURN ret,
                       SQLTCHAR* msg_return = NULL, const SQLSMALLINT sz = 0);
bool CheckSQLSTATE(SQLSMALLINT handle_type, SQLHANDLE handle,
                   SQLWCHAR* expected_sqlstate, bool log_message);
bool CheckSQLSTATE(SQLSMALLINT handle_type, SQLHANDLE handle,
                   SQLWCHAR* expected_sqlstate);
std::wstring QueryBuilder(const std::wstring& column,
                          const std::wstring& dataset,
                          const std::wstring& count);
std::wstring QueryBuilder(const std::wstring& column,
                          const std::wstring& dataset);
void CloseCursor(SQLHSTMT* h_statement, bool throw_on_error, bool log_diag);
std::string u16string_to_string(const std::u16string& src);
std::u16string string_to_u16string(const std::string& src);

#endif
