
#include <gtest/gtest.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "CDRGenerator.h"

using namespace std;

class RegressionTest : public ::testing::TestWithParam<std::string> { };

std::vector<std::string> filenames;

TEST_P(RegressionTest, RegressionTestCase) {
    std::string root = GetParam();
    std::string input_path = "input/" + root + ".idl";
    std::string output_path = "output/" + root + ".c";

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
    rv = parse(input_file, observed_output_file_w, observed_error);
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
  string operator()(const testing::TestParamInfo<string>& info) const {
    return info.param;
  }
};

INSTANTIATE_TEST_SUITE_P(RegressionTestSuite,
    RegressionTest, ::testing::ValuesIn(filenames),
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
