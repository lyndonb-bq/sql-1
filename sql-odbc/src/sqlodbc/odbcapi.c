/*
 * Copyright OpenSearch Contributors
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdio.h>
#include <string.h>

#include "environ.h"
#include "opensearch_odbc.h"
#include "loadlib.h"
#include "misc.h"
#include "opensearch_apifunc.h"
#include "opensearch_connection.h"
#include "opensearch_driver_connect.h"
#include "opensearch_info.h"
#include "opensearch_statement.h"
#include "qresult.h"
#include "statement.h"

BOOL SC_connection_lost_check(StatementClass *stmt, const char *funcname) {
    ConnectionClass *conn = SC_get_conn(stmt);
    char message[64];

    if (NULL != conn->opensearchconn)
        return FALSE;
    SC_clear_error(stmt);
    SPRINTF_FIXED(message, "%s unable due to the connection lost", funcname);
    SC_set_error(stmt, STMT_COMMUNICATION_ERROR, message, funcname);
    return TRUE;
}

RETCODE SQL_API SQLBindCol(HSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                           SQLSMALLINT TargetType, PTR TargetValue,
                           SQLLEN BufferLength, SQLLEN *StrLen_or_Ind) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_BindCol(StatementHandle, ColumnNumber, TargetType, TargetValue,
                        BufferLength, StrLen_or_Ind);
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLCancel(HSTMT StatementHandle) {
    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (!StatementHandle)
        return SQL_INVALID_HANDLE;
    if (SC_connection_lost_check((StatementClass *)StatementHandle,
                                 __FUNCTION__))
        return SQL_ERROR;
    return OPENSEARCHAPI_Cancel(StatementHandle);
}

static BOOL theResultIsEmpty(const StatementClass *stmt) {
    QResultClass *res = SC_get_Result(stmt);
    if (NULL == res)
        return FALSE;
    return (0 == QR_get_num_total_tuples(res));
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLColumns(HSTMT StatementHandle, SQLCHAR *CatalogName,
                           SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
                           SQLSMALLINT NameLength2, SQLCHAR *TableName,
                           SQLSMALLINT NameLength3, SQLCHAR *ColumnName,
                           SQLSMALLINT NameLength4) {
    CSTR func = "SQLColumns";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName,
            *clName = ColumnName;
    UWORD flag = PODBC_SEARCH_PUBLIC_SCHEMA;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_Columns(StatementHandle, ctName, NameLength1, scName,
                            NameLength2, tbName, NameLength3, clName,
                            NameLength4, flag, 0, 0);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL, *newCl = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb =
                make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (newCl = make_lstring_ifneeded(conn, ColumnName, NameLength4,
                                          ifallupper),
            NULL != newCl) {
            clName = newCl;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_Columns(StatementHandle, ctName, NameLength1, scName,
                                NameLength2, tbName, NameLength3, clName,
                                NameLength4, flag, 0, 0);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
            if (newCl)
                free(newCl);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLConnect(HDBC ConnectionHandle, SQLCHAR *ServerName,
                           SQLSMALLINT NameLength1, SQLCHAR *UserName,
                           SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                           SQLSMALLINT NameLength3) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)ConnectionHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret = OPENSEARCHAPI_Connect(ConnectionHandle, ServerName, NameLength1, UserName,
                        NameLength2, Authentication, NameLength3);
    LEAVE_CONN_CS(conn);
    return ret;
}

RETCODE SQL_API SQLDriverConnect(HDBC hdbc, HWND hwnd, SQLCHAR *szConnStrIn,
                                 SQLSMALLINT cbConnStrIn, SQLCHAR *szConnStrOut,
                                 SQLSMALLINT cbConnStrOutMax,
                                 SQLSMALLINT *pcbConnStrOut,
                                 SQLUSMALLINT fDriverCompletion) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)hdbc;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret =
        OPENSEARCHAPI_DriverConnect(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
                            cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    LEAVE_CONN_CS(conn);
    return ret;
}
RETCODE SQL_API SQLBrowseConnect(HDBC hdbc, SQLCHAR *szConnStrIn,
                                 SQLSMALLINT cbConnStrIn, SQLCHAR *szConnStrOut,
                                 SQLSMALLINT cbConnStrOutMax,
                                 SQLSMALLINT *pcbConnStrOut) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)hdbc;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret = OPENSEARCHAPI_BrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
                              cbConnStrOutMax, pcbConnStrOut);
    LEAVE_CONN_CS(conn);
    return ret;
}

RETCODE SQL_API SQLDataSources(HENV EnvironmentHandle, SQLUSMALLINT Direction,
                               SQLCHAR *ServerName, SQLSMALLINT BufferLength1,
                               SQLSMALLINT *NameLength1, SQLCHAR *Description,
                               SQLSMALLINT BufferLength2,
                               SQLSMALLINT *NameLength2) {
    UNUSED(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1,
           Description, BufferLength2, NameLength2);
    MYLOG(OPENSEARCH_TRACE, "entering\n");
    return SQL_ERROR;
}

RETCODE SQL_API SQLDescribeCol(HSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                               SQLCHAR *ColumnName, SQLSMALLINT BufferLength,
                               SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                               SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits,
                               SQLSMALLINT *Nullable) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_DescribeCol(StatementHandle, ColumnNumber, ColumnName,
                            BufferLength, NameLength, DataType, ColumnSize,
                            DecimalDigits, Nullable);
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLDisconnect(HDBC ConnectionHandle) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)ConnectionHandle;

    MYLOG(OPENSEARCH_TRACE, "entering for %p\n", ConnectionHandle);
#ifdef _HANDLE_ENLIST_IN_DTC_
    if (CC_is_in_global_trans(conn))
        CALL_DtcOnDisconnect(conn);
#endif /* _HANDLE_ENLIST_IN_DTC_ */
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret = OPENSEARCHAPI_Disconnect(ConnectionHandle);
    LEAVE_CONN_CS(conn);
    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLExecDirect(HSTMT StatementHandle, SQLCHAR *StatementText,
                              SQLINTEGER TextLength) {
    if (StatementHandle == NULL)
        return SQL_ERROR;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    // Enter critical
    ENTER_STMT_CS(stmt);

    // Clear error and rollback
    SC_clear_error(stmt);

    // Execute statement if statement is ready
    RETCODE ret = SQL_ERROR;
    if (!SC_opencheck(stmt, "SQLExecDirect"))
        ret = OPENSEARCHAPI_ExecDirect(StatementHandle, StatementText, TextLength, 1);

    // Exit critical
    LEAVE_STMT_CS(stmt);

    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLExecute(HSTMT StatementHandle) {
    if (StatementHandle == NULL)
        return SQL_ERROR;

    StatementClass *stmt = (StatementClass *)StatementHandle;
    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    // Enter critical
    ENTER_STMT_CS(stmt);

    // Clear error and rollback
    SC_clear_error(stmt);
    RETCODE ret = SQL_ERROR;
    if (!SC_opencheck(stmt, "SQLExecute"))
        ret = OPENSEARCHAPI_Execute(StatementHandle);

    // Exit critical
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLFetch(HSTMT StatementHandle) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    IRDFields *irdopts = SC_get_IRDF(stmt);
    ARDFields *ardopts = SC_get_ARDF(stmt);
    SQLUSMALLINT *rowStatusArray = irdopts->rowStatusArray;
    SQLULEN *pcRow = irdopts->rowsFetched;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_ExtendedFetch(StatementHandle, SQL_FETCH_NEXT, 0, pcRow,
                              rowStatusArray, 0, ardopts->size_of_rowset);
    stmt->transition_status = STMT_TRANSITION_FETCH_SCROLL;

    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLFreeStmt(HSTMT StatementHandle, SQLUSMALLINT Option) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    ConnectionClass *conn = NULL;

    MYLOG(OPENSEARCH_TRACE, "entering\n");

    if (stmt) {
        if (Option == SQL_DROP) {
            conn = stmt->hdbc;
            if (conn)
                ENTER_CONN_CS(conn);
        } else
            ENTER_STMT_CS(stmt);
    }

    ret = OPENSEARCHAPI_FreeStmt(StatementHandle, Option);

    if (stmt) {
        if (Option == SQL_DROP) {
            if (conn)
                LEAVE_CONN_CS(conn);
        } else
            LEAVE_STMT_CS(stmt);
    }

    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLGetCursorName(HSTMT StatementHandle, SQLCHAR *CursorName,
                                 SQLSMALLINT BufferLength,
                                 SQLSMALLINT *NameLength) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_GetCursorName(StatementHandle, CursorName, BufferLength,
                              NameLength);
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLGetData(HSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                           SQLSMALLINT TargetType, PTR TargetValue,
                           SQLLEN BufferLength, SQLLEN *StrLen_or_Ind) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_GetData(StatementHandle, ColumnNumber, TargetType, TargetValue,
                        BufferLength, StrLen_or_Ind);
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLGetFunctions(HDBC ConnectionHandle, SQLUSMALLINT FunctionId,
                                SQLUSMALLINT *Supported) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)ConnectionHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    if (FunctionId == SQL_API_ODBC3_ALL_FUNCTIONS)
        ret = OPENSEARCHAPI_GetFunctions30(ConnectionHandle, FunctionId, Supported);
    else
        ret = OPENSEARCHAPI_GetFunctions(ConnectionHandle, FunctionId, Supported);

    LEAVE_CONN_CS(conn);
    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLGetInfo(HDBC ConnectionHandle, SQLUSMALLINT InfoType,
                           PTR InfoValue, SQLSMALLINT BufferLength,
                           SQLSMALLINT *StringLength) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)ConnectionHandle;

    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if ((ret = OPENSEARCHAPI_GetInfo(ConnectionHandle, InfoType, InfoValue,
                             BufferLength, StringLength))
        == SQL_ERROR)
        CC_log_error("SQLGetInfo(30)", "", conn);
    LEAVE_CONN_CS(conn);
    return ret;
}

RETCODE SQL_API SQLGetTypeInfo(HSTMT StatementHandle, SQLSMALLINT DataType) {
    CSTR func = "SQLGetTypeInfo";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check((StatementClass *)StatementHandle,
                                 __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_GetTypeInfo(StatementHandle, DataType);
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLNumResultCols(HSTMT StatementHandle,
                                 SQLSMALLINT *ColumnCount) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_NumResultCols(StatementHandle, ColumnCount);
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLParamData(HSTMT StatementHandle, PTR *Value) {
    UNUSED(Value);
    StatementClass *stmt = (StatementClass *)StatementHandle;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "OpenSearch does not support parameters.", "SQLParamData");
    return SQL_ERROR;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLPrepare(HSTMT StatementHandle, SQLCHAR *StatementText,
                           SQLINTEGER TextLength) {
    if (StatementHandle == NULL)
        return SQL_ERROR;

    CSTR func = "SQLPrepare";
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    // Enter critical
    ENTER_STMT_CS(stmt);

    // Clear error and rollback
    SC_clear_error(stmt);

    // Prepare statement if statement is ready
    RETCODE ret = SQL_ERROR;
    if (!SC_opencheck(stmt, func))
        ret = OPENSEARCHAPI_Prepare(StatementHandle, StatementText, TextLength);

    // Exit critical
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLPutData(HSTMT StatementHandle, PTR Data,
                           SQLLEN StrLen_or_Ind) {
    UNUSED(Data, StrLen_or_Ind);
    StatementClass *stmt = (StatementClass *)StatementHandle;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "OpenSearch does not support parameters.", "SQLPutData");
    return SQL_ERROR;
}

RETCODE SQL_API SQLRowCount(HSTMT StatementHandle, SQLLEN *RowCount) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_RowCount(StatementHandle, RowCount);
    LEAVE_STMT_CS(stmt);
    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLSetCursorName(HSTMT StatementHandle, SQLCHAR *CursorName,
                                 SQLSMALLINT NameLength) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_SetCursorName(StatementHandle, CursorName, NameLength);
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLSetParam(HSTMT StatementHandle, SQLUSMALLINT ParameterNumber,
                            SQLSMALLINT ValueType, SQLSMALLINT ParameterType,
                            SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                            PTR ParameterValue, SQLLEN *StrLen_or_Ind) {
    UNUSED(ParameterNumber, ValueType, ParameterType, LengthPrecision,
           ParameterScale, ParameterValue, StrLen_or_Ind);
    StatementClass *stmt = (StatementClass *)StatementHandle;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "OpenSearch does not support parameters.", "SQLSetParam");
    return SQL_ERROR;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLSpecialColumns(HSTMT StatementHandle,
                                  SQLUSMALLINT IdentifierType,
                                  SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                  SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                  SQLCHAR *TableName, SQLSMALLINT NameLength3,
                                  SQLUSMALLINT Scope, SQLUSMALLINT Nullable) {
    CSTR func = "SQLSpecialColumns";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_SpecialColumns(StatementHandle, IdentifierType, ctName,
                                   NameLength1, scName, NameLength2, tbName,
                                   NameLength3, Scope, Nullable);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb =
                make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_SpecialColumns(StatementHandle, IdentifierType, ctName,
                                       NameLength1, scName, NameLength2, tbName,
                                       NameLength3, Scope, Nullable);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLStatistics(HSTMT StatementHandle, SQLCHAR *CatalogName,
                              SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
                              SQLSMALLINT NameLength2, SQLCHAR *TableName,
                              SQLSMALLINT NameLength3, SQLUSMALLINT Unique,
                              SQLUSMALLINT Reserved) {
    CSTR func = "SQLStatistics";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_Statistics(StatementHandle, ctName, NameLength1, scName,
                               NameLength2, tbName, NameLength3, Unique,
                               Reserved);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb =
                make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_Statistics(StatementHandle, ctName, NameLength1, scName,
                                   NameLength2, tbName, NameLength3, Unique,
                                   Reserved);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLTables(HSTMT StatementHandle, SQLCHAR *CatalogName,
                          SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
                          SQLSMALLINT NameLength2, SQLCHAR *TableName,
                          SQLSMALLINT NameLength3, SQLCHAR *TableType,
                          SQLSMALLINT NameLength4) {
    CSTR func = "SQLTables";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;
    UWORD flag = 0;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_Tables(StatementHandle, ctName, NameLength1, scName,
                           NameLength2, tbName, NameLength3, TableType,
                           NameLength4, flag);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb =
                make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_Tables(StatementHandle, ctName, NameLength1, scName,
                               NameLength2, tbName, NameLength3, TableType,
                               NameLength4, flag);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLColumnPrivileges(
    HSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
    SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
    SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName) {
    CSTR func = "SQLColumnPrivileges";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
            *tbName = szTableName, *clName = szColumnName;
    UWORD flag = 0;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_ColumnPrivileges(hstmt, ctName, cbCatalogName, scName,
                                     cbSchemaName, tbName, cbTableName, clName,
                                     cbColumnName, flag);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL, *newCl = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb = make_lstring_ifneeded(conn, szTableName, cbTableName,
                                          ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (newCl = make_lstring_ifneeded(conn, szColumnName, cbColumnName,
                                          ifallupper),
            NULL != newCl) {
            clName = newCl;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_ColumnPrivileges(hstmt, ctName, cbCatalogName, scName,
                                         cbSchemaName, tbName, cbTableName,
                                         clName, cbColumnName, flag);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
            if (newCl)
                free(newCl);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLDescribeParam(HSTMT hstmt, SQLUSMALLINT ipar,
                                 SQLSMALLINT *pfSqlType, SQLULEN *pcbParamDef,
                                 SQLSMALLINT *pibScale,
                                 SQLSMALLINT *pfNullable) {
    UNUSED(ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
    StatementClass *stmt = (StatementClass *)hstmt;
    SC_clear_error(stmt);

    // COLNUM_ERROR translates to 'invalid descriptor index'
    SC_set_error(stmt, STMT_COLNUM_ERROR,
                 "OpenSearch does not support parameters.", "SQLNumParams");
    return SQL_ERROR;
}

RETCODE SQL_API SQLExtendedFetch(HSTMT hstmt, SQLUSMALLINT fFetchType,
                                 SQLLEN irow,
#if defined(WITH_UNIXODBC) && (SIZEOF_LONG_INT != 8)
                                 SQLROWSETSIZE *pcrow,
#else
                                 SQLULEN *pcrow,
#endif /* WITH_UNIXODBC */
                                 SQLUSMALLINT *rgfRowStatus) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
#ifdef WITH_UNIXODBC
    {
        SQLULEN retrieved;

        ret = OPENSEARCHAPI_ExtendedFetch(hstmt, fFetchType, irow, &retrieved,
                                  rgfRowStatus, 0,
                                  SC_get_ARDF(stmt)->size_of_rowset_odbc2);
        if (pcrow)
            *pcrow = retrieved;
    }
#else
    ret = OPENSEARCHAPI_ExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus, 0,
                              SC_get_ARDF(stmt)->size_of_rowset_odbc2);
#endif /* WITH_UNIXODBC */
    stmt->transition_status = STMT_TRANSITION_EXTENDED_FETCH;
    LEAVE_STMT_CS(stmt);
    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLForeignKeys(
    HSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
    SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
    SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
    SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
    SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName,
    SQLSMALLINT cbFkTableName) {
    CSTR func = "SQLForeignKeys";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *pkctName = szPkCatalogName, *pkscName = szPkSchemaName,
            *pktbName = szPkTableName, *fkctName = szFkCatalogName,
            *fkscName = szFkSchemaName, *fktbName = szFkTableName;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_ForeignKeys(hstmt, pkctName, cbPkCatalogName, pkscName,
                                cbPkSchemaName, pktbName, cbPkTableName,
                                fkctName, cbFkCatalogName, fkscName,
                                cbFkSchemaName, fktbName, cbFkTableName);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newPkct = NULL, *newPksc = NULL, *newPktb = NULL,
                *newFkct = NULL, *newFksc = NULL, *newFktb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newPkct = make_lstring_ifneeded(conn, szPkCatalogName,
                                            cbPkCatalogName, ifallupper),
            NULL != newPkct) {
            pkctName = newPkct;
            reexec = TRUE;
        }
        if (newPksc = make_lstring_ifneeded(conn, szPkSchemaName,
                                            cbPkSchemaName, ifallupper),
            NULL != newPksc) {
            pkscName = newPksc;
            reexec = TRUE;
        }
        if (newPktb = make_lstring_ifneeded(conn, szPkTableName, cbPkTableName,
                                            ifallupper),
            NULL != newPktb) {
            pktbName = newPktb;
            reexec = TRUE;
        }
        if (newFkct = make_lstring_ifneeded(conn, szFkCatalogName,
                                            cbFkCatalogName, ifallupper),
            NULL != newFkct) {
            fkctName = newFkct;
            reexec = TRUE;
        }
        if (newFksc = make_lstring_ifneeded(conn, szFkSchemaName,
                                            cbFkSchemaName, ifallupper),
            NULL != newFksc) {
            fkscName = newFksc;
            reexec = TRUE;
        }
        if (newFktb = make_lstring_ifneeded(conn, szFkTableName, cbFkTableName,
                                            ifallupper),
            NULL != newFktb) {
            fktbName = newFktb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_ForeignKeys(hstmt, pkctName, cbPkCatalogName, pkscName,
                                    cbPkSchemaName, pktbName, cbPkTableName,
                                    fkctName, cbFkCatalogName, fkscName,
                                    cbFkSchemaName, fktbName, cbFkTableName);
            if (newPkct)
                free(newPkct);
            if (newPksc)
                free(newPksc);
            if (newPktb)
                free(newPktb);
            if (newFkct)
                free(newFkct);
            if (newFksc)
                free(newFksc);
            if (newFktb)
                free(newFktb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLMoreResults(HSTMT hstmt) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_MoreResults(hstmt);
    LEAVE_STMT_CS(stmt);
    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLNativeSql(HDBC hdbc, SQLCHAR *szSqlStrIn,
                             SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                             SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)hdbc;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret = OPENSEARCHAPI_NativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax,
                          pcbSqlStr);
    LEAVE_CONN_CS(conn);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLNumParams(HSTMT hstmt, SQLSMALLINT *pcpar) {
    if (pcpar != NULL)
        *pcpar = 0;

    StatementClass *stmt = (StatementClass *)hstmt;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "OpenSearch does not support parameters.", "SQLNumParams");
    return SQL_SUCCESS_WITH_INFO;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLPrimaryKeys(HSTMT hstmt, SQLCHAR *szCatalogName,
                               SQLSMALLINT cbCatalogName, SQLCHAR *szSchemaName,
                               SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                               SQLSMALLINT cbTableName) {
    CSTR func = "SQLPrimaryKeys";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
            *tbName = szTableName;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_PrimaryKeys(hstmt, ctName, cbCatalogName, scName,
                                cbSchemaName, tbName, cbTableName, 0);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb = make_lstring_ifneeded(conn, szTableName, cbTableName,
                                          ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_PrimaryKeys(hstmt, ctName, cbCatalogName, scName,
                                    cbSchemaName, tbName, cbTableName, 0);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLProcedureColumns(
    HSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
    SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
    SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName) {
    CSTR func = "SQLProcedureColumns";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
            *prName = szProcName, *clName = szColumnName;
    UWORD flag = 0;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_ProcedureColumns(hstmt, ctName, cbCatalogName, scName,
                                     cbSchemaName, prName, cbProcName, clName,
                                     cbColumnName, flag);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newPr = NULL, *newCl = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newPr =
                make_lstring_ifneeded(conn, szProcName, cbProcName, ifallupper),
            NULL != newPr) {
            prName = newPr;
            reexec = TRUE;
        }
        if (newCl = make_lstring_ifneeded(conn, szColumnName, cbColumnName,
                                          ifallupper),
            NULL != newCl) {
            clName = newCl;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_ProcedureColumns(hstmt, ctName, cbCatalogName, scName,
                                         cbSchemaName, prName, cbProcName,
                                         clName, cbColumnName, flag);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newPr)
                free(newPr);
            if (newCl)
                free(newCl);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}

RETCODE SQL_API SQLProcedures(HSTMT hstmt, SQLCHAR *szCatalogName,
                              SQLSMALLINT cbCatalogName, SQLCHAR *szSchemaName,
                              SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                              SQLSMALLINT cbProcName) {
    CSTR func = "SQLProcedures";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
            *prName = szProcName;
    UWORD flag = 0;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_Procedures(hstmt, ctName, cbCatalogName, scName,
                               cbSchemaName, prName, cbProcName, flag);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newPr = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newPr =
                make_lstring_ifneeded(conn, szProcName, cbProcName, ifallupper),
            NULL != newPr) {
            prName = newPr;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_Procedures(hstmt, ctName, cbCatalogName, scName,
                                   cbSchemaName, prName, cbProcName, flag);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newPr)
                free(newPr);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLSetPos(HSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption,
                          SQLUSMALLINT fLock) {
    UNUSED(irow, fOption, fLock);
    StatementClass *stmt = (StatementClass *)hstmt;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "SQLSetPos is not supported.", "SQLSetPos");
    return SQL_ERROR;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLTablePrivileges(HSTMT hstmt, SQLCHAR *szCatalogName,
                                   SQLSMALLINT cbCatalogName,
                                   SQLCHAR *szSchemaName,
                                   SQLSMALLINT cbSchemaName,
                                   SQLCHAR *szTableName,
                                   SQLSMALLINT cbTableName) {
    CSTR func = "SQLTablePrivileges";
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)hstmt;
    SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
            *tbName = szTableName;
    UWORD flag = 0;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    if (stmt->options.metadata_id)
        flag |= PODBC_NOT_SEARCH_PATTERN;
    if (SC_opencheck(stmt, func))
        ret = SQL_ERROR;
    else
        ret = OPENSEARCHAPI_TablePrivileges(hstmt, ctName, cbCatalogName, scName,
                                    cbSchemaName, tbName, cbTableName, flag);
    if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
        BOOL ifallupper = TRUE, reexec = FALSE;
        SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
        ConnectionClass *conn = SC_get_conn(stmt);

        if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                          ifallupper),
            NULL != newCt) {
            ctName = newCt;
            reexec = TRUE;
        }
        if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                          ifallupper),
            NULL != newSc) {
            scName = newSc;
            reexec = TRUE;
        }
        if (newTb = make_lstring_ifneeded(conn, szTableName, cbTableName,
                                          ifallupper),
            NULL != newTb) {
            tbName = newTb;
            reexec = TRUE;
        }
        if (reexec) {
            ret = OPENSEARCHAPI_TablePrivileges(hstmt, ctName, cbCatalogName, scName,
                                        cbSchemaName, tbName, cbTableName, 0);
            if (newCt)
                free(newCt);
            if (newSc)
                free(newSc);
            if (newTb)
                free(newTb);
        }
    }
    LEAVE_STMT_CS(stmt);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */

RETCODE SQL_API SQLBindParameter(HSTMT hstmt, SQLUSMALLINT ipar,
                                 SQLSMALLINT fParamType, SQLSMALLINT fCType,
                                 SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                 SQLSMALLINT ibScale, PTR rgbValue,
                                 SQLLEN cbValueMax, SQLLEN *pcbValue) {
    UNUSED(ipar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue,
           cbValueMax, pcbValue);
    StatementClass *stmt = (StatementClass *)hstmt;
    if (stmt == NULL)
        return SQL_ERROR;
    SC_clear_error(stmt);
    SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR,
                 "OpenSearch does not support parameters.",
                 "SQLBindParameter");
    return SQL_ERROR;
}

/* ODBC 2.x-specific functions */
// TODO (#590): Add implementations for remaining ODBC 2.x function

RETCODE SQL_API SQLAllocStmt(SQLHDBC InputHandle, SQLHSTMT *OutputHandle) {
    RETCODE ret;
    ConnectionClass *conn;
    MYLOG(OPENSEARCH_TRACE, "entering\n");

    conn = (ConnectionClass *)InputHandle;
    ENTER_CONN_CS(conn);
    ret = OPENSEARCHAPI_AllocStmt(
        InputHandle, OutputHandle,
        PODBC_EXTERNAL_STATEMENT | PODBC_INHERIT_CONNECT_OPTIONS);
    if (*OutputHandle)
        ((StatementClass *)(*OutputHandle))->external = 1;
    LEAVE_CONN_CS(conn);

    return ret;
}

#ifndef UNICODE_SUPPORTXX
RETCODE SQL_API SQLGetConnectOption(HDBC ConnectionHandle, SQLUSMALLINT Option,
                                    PTR Value) {
    RETCODE ret;

    MYLOG(OPENSEARCH_TRACE, "entering " FORMAT_UINTEGER "\n", Option);
    ENTER_CONN_CS((ConnectionClass *)ConnectionHandle);
    CC_clear_error((ConnectionClass *)ConnectionHandle);
    ret = OPENSEARCHAPI_GetConnectOption(ConnectionHandle, Option, Value, NULL, 0);
    LEAVE_CONN_CS((ConnectionClass *)ConnectionHandle);
    return ret;
}

/*	SQLSetConnectOption -> SQLSetConnectAttr */
RETCODE SQL_API SQLSetConnectOption(HDBC ConnectionHandle, SQLUSMALLINT Option,
                                    SQLULEN Value) {
    RETCODE ret;
    ConnectionClass *conn = (ConnectionClass *)ConnectionHandle;

    MYLOG(OPENSEARCH_TRACE, "entering " FORMAT_INTEGER "\n", Option);
    ENTER_CONN_CS(conn);
    CC_clear_error(conn);
    ret = OPENSEARCHAPI_SetConnectOption(ConnectionHandle, Option, Value);
    LEAVE_CONN_CS(conn);
    return ret;
}

/*	SQLColAttributes -> SQLColAttribute */
SQLRETURN SQL_API SQLColAttributes(SQLHSTMT StatementHandle,
                                   SQLUSMALLINT ColumnNumber,
                                   SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER CharacterAttribute,
                                   SQLSMALLINT BufferLength,
                                   SQLSMALLINT *StringLength,
#if defined(_WIN64) || defined(_WIN32) || defined(SQLCOLATTRIBUTE_SQLLEN)
                                   SQLLEN *NumericAttribute
#else
                                   SQLPOINTER NumericAttribute
#endif
) {
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;

    MYLOG(OPENSEARCH_TRACE, "entering\n");
    if (SC_connection_lost_check(stmt, __FUNCTION__))
        return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    SC_clear_error(stmt);
    ret = OPENSEARCHAPI_ColAttributes(StatementHandle, ColumnNumber, FieldIdentifier,
                              CharacterAttribute, BufferLength, StringLength,
                              NumericAttribute);
    LEAVE_STMT_CS(stmt);
    return ret;
}

/*	SQLError -> SQLDiagRec */
RETCODE SQL_API SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle,
                         SQLHSTMT StatementHandle, SQLCHAR *Sqlstate,
                         SQLINTEGER *NativeError, SQLCHAR *MessageText,
                         SQLSMALLINT BufferLength, SQLSMALLINT *TextLength) {
    RETCODE ret;
    SQLSMALLINT RecNumber = 1;

    MYLOG(OPENSEARCH_TRACE, "entering\n");

    if (StatementHandle) {
        ret =
            OPENSEARCHAPI_StmtError(StatementHandle, RecNumber, Sqlstate, NativeError,
                           MessageText, BufferLength, TextLength, 0);
    } else if (ConnectionHandle) {
        ret = OPENSEARCHAPI_ConnectError(ConnectionHandle, RecNumber, Sqlstate,
                                 NativeError, MessageText, BufferLength,
                                 TextLength, 0);
    } else if (EnvironmentHandle) {
        ret = OPENSEARCHAPI_EnvError(EnvironmentHandle, RecNumber, Sqlstate, NativeError,
                              MessageText, BufferLength, TextLength, 0);
    } else {
        ret = SQL_ERROR;
    }

    MYLOG(OPENSEARCH_TRACE, "leaving %d\n", ret);
    return ret;
}
#endif /* UNICODE_SUPPORTXX */
