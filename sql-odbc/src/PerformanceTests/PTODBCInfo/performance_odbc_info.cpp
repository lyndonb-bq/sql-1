/*
 * Copyright OpenSearch Contributors
 * SPDX-License-Identifier: Apache-2.0
 */


// clang-format off
#include "pch.h"
#include "opensearch_communication.h"
#include "unit_test_helper.h"
#include "it_odbc_helper.h"
#include "chrono"
#include <cwctype>
#include "rabbit.hpp"
#include <codecvt>
#include <locale>
// clang-format on

const std::vector< std::string > base_items = {"name", "cluster_name",
                                               "cluster_uuid"};
const std::vector< std::string > version_items = {
    "number",
    "build_flavor",
    "build_type",
    "build_hash",
    "build_date",
    "build_snapshot",
    "lucene_version",
    "minimum_wire_compatibility_version",
    "minimum_index_compatibility_version"};
const std::string sync_start = "%%__PARSE__SYNC__START__%%";
const std::string sync_sep = "%%__SEP__%%";
const std::string sync_end = "%%__PARSE__SYNC__END__%%";

std::string wstring_to_string(const std::wstring& src) {
    return std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t >{}
        .to_bytes(src);
}

runtime_options rt_opts = []() {
    runtime_options temp_opts;
    for (auto it : conn_str_pair) {
        std::wstring tmp = it.first;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), towlower);
        if (tmp == L"host")
            temp_opts.conn.server = wstring_to_string(it.second);
        else if (tmp == L"port")
            temp_opts.conn.port = wstring_to_string(it.second);
        else if (tmp == L"responsetimeout")
            temp_opts.conn.timeout = wstring_to_string(it.second);
        else if (tmp == L"auth")
            temp_opts.auth.auth_type = wstring_to_string(it.second);
        else if (tmp == L"user")
            temp_opts.auth.username = wstring_to_string(it.second);
        else if (tmp == L"password")
            temp_opts.auth.password = wstring_to_string(it.second);
        else if (tmp == L"region")
            temp_opts.auth.region = wstring_to_string(it.second);
        else if (tmp == L"usessl")
            temp_opts.crypt.use_ssl =
                (std::stoul(it.second, nullptr, 10) ? true : false);
        else if (tmp == L"")
            temp_opts.crypt.verify_server =
                (std::stoul(it.second, nullptr, 10) ? true : false);
    }
    return temp_opts;
}();

void GetVersionInfoString(std::string& version_info) {
    // Connect to DB
    OpenSearchCommunication opensearch_comm;
    opensearch_comm.ConnectionOptions(rt_opts, false, 0, 0);
    ASSERT_TRUE(opensearch_comm.ConnectDBStart());

    // Issue request
    std::string endpoint, content_type, query, fetch_size;
    std::shared_ptr< Aws::Http::HttpResponse > response =
        opensearch_comm.IssueRequest(endpoint, Aws::Http::HttpMethod::HTTP_GET,
                             content_type, query, fetch_size);

    // Convert response to string
    ASSERT_TRUE(response != nullptr);
    opensearch_comm.AwsHttpResponseToString(response, version_info);
}

void ParseVersionInfoString(
    const std::string& input_info,
    std::vector< std::pair< std::string, std::string > >& output_info) {
    // Parse input
    rabbit::document doc;
    doc.parse(input_info);

    // Populate output with info
    for (auto& it : base_items) {
        ASSERT_TRUE(doc.has(it));
        output_info.push_back(
            std::pair< std::string, std::string >(it, doc[it].str()));
    }

    ASSERT_TRUE(doc.has("version"));
    for (auto& it : version_items) {
        ASSERT_TRUE(doc["version"].has(it));
        output_info.push_back(std::pair< std::string, std::string >(
            it, doc["version"][it].str()));
    }
}

TEST(InfoCollect, EndPoint) {
    // Get version string from endpoint
    std::string version_info;
    GetVersionInfoString(version_info);

    // Parse into vector pair
    std::vector< std::pair< std::string, std::string > > output_info;
    ParseVersionInfoString(version_info, output_info);

    std::cout << sync_start << std::endl;
    for (auto& it : output_info) {
        std::cout << it.first << sync_sep << it.second << std::endl;
    }
    std::cout << sync_end << std::endl;
}

int main(int argc, char** argv) {
    testing::internal::CaptureStdout();
    ::testing::InitGoogleTest(&argc, argv);

    int failures = RUN_ALL_TESTS();

    std::string output = testing::internal::GetCapturedStdout();
    std::cout << output << std::endl;
    std::cout << (failures ? "Not all tests passed." : "All tests passed")
              << std::endl;
    WriteFileIfSpecified(argv, argv + argc, "-fout", output);

    return failures;
}
