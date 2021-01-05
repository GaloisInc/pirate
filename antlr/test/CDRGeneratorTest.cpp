/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <gtest/gtest.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "CDRGenerator.hpp"

using namespace std;

class RegressionTest : public ::testing::TestWithParam<std::tuple<std::string, target_t, bool>> { };

std::vector<std::string> filenames;

TEST_P(RegressionTest, RegressionTestCase) {
    auto params = GetParam();
    std::string root = std::get<0>(params);
    target_t target = std::get<1>(params);
    bool packed = std::get<2>(params);
    std::string input_path = "input/" + root + ".idl";
    std::string packed_path = packed ? "/packed" : "";
    std::string output_path = "output/" + target_as_string(target) + packed_path + "/" + root + "." + target_as_string(target);

    ifstream input_file;
    ifstream expected_output_file;
    stringstream expected_output;

    // ostream_indenter does not work on stringstream
    ofstream observed_output_file_w;
    ifstream observed_output_file_r;
    stringstream observed_output;

    stringstream observed_error;
    int rv;

    input_file.open(input_path);
    expected_output_file.open(output_path);
    observed_output_file_w.open("/tmp/" + root + ".c");
    ASSERT_FALSE(input_file.fail()) << input_path;
    ASSERT_FALSE(expected_output_file.fail());
    ASSERT_FALSE(observed_output_file_w.fail());
    expected_output << expected_output_file.rdbuf();
    expected_output_file.close();
    rv = parse(input_file, observed_output_file_w, observed_error, target, packed);
    input_file.close();
    observed_output_file_w.close();
    observed_output_file_r.open("/tmp/" + root + ".c");
    ASSERT_FALSE(observed_output_file_r.fail());
    observed_output << observed_output_file_r.rdbuf();
    observed_output_file_r.close();
    ASSERT_EQ("", observed_error.str()) << observed_error.str();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(expected_output.str(), observed_output.str());
}

struct PrintParamName {
  string operator()(const testing::TestParamInfo<std::tuple<std::string, target_t, bool>>& info) const {
    string packed = std::get<2>(info.param) ? "_packed" : "";
    return std::get<0>(info.param) + "_" + target_as_string(std::get<1>(info.param)) + packed;
  }
};

INSTANTIATE_TEST_SUITE_P(RegressionTestSuite,
    RegressionTest, ::testing::Combine(::testing::ValuesIn(filenames),
        ::testing::Values(TargetLanguage::C_LANG, TargetLanguage::CPP_LANG),
        ::testing::Values(false, true)),
    PrintParamName());

void initial_setup() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("input")) == NULL) {
        FAIL() << "input directory " << strerror(errno);
    }
    while ((ent = readdir(dir)) != NULL) {
        char root[32];
        if ((strncmp(ent->d_name, ".", 2) == 0) || (strncmp(ent->d_name, "..", 3) == 0)) {
            continue;
        }
        strncpy(root, ent->d_name, 32);
        char *loc = strchr(root, '.');
        ASSERT_NE(nullptr, loc);
        *loc = 0;
        filenames.push_back(root);
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    initial_setup();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
