
#include <gtest/gtest.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "CDRGenerator.h"

using namespace std;

void regressionTest(string input_path, string output_path) {
    ifstream input_file;
    ifstream expected_output_file;
    stringstream expected_output;    
    stringstream observed_output;
    stringstream observed_error;
    int rv;

    input_file.open(input_path);
    expected_output_file.open(output_path);
    ASSERT_FALSE(input_file.fail()) << input_path;
    ASSERT_FALSE(expected_output_file.fail());
    expected_output << expected_output_file.rdbuf();
    expected_output_file.close();
    rv = parse(input_file, observed_output, observed_error);
    input_file.close();
    ASSERT_EQ("", observed_error.str()) << observed_error.str();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(expected_output.str(), observed_output.str());
}


TEST(CDRGeneratorTest, IdlRegressionTests) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("input")) == NULL) {
        FAIL() << "input " << strerror(errno);
    }
    while ((ent = readdir(dir)) != NULL) {
        char source[64];
        char sink[64];
        char root[32];
        if ((strncmp(ent->d_name, ".", 2) == 0) || (strncmp(ent->d_name, "..", 3) == 0)) {
            continue;
        }
        strncpy(root, ent->d_name, 64);
        char *loc = strchr(root, '.');
        ASSERT_NE(nullptr, loc);
        *loc = 0;
        snprintf(source, 64, "input/%s.idl", root);
        snprintf(sink, 64, "output/%s.c", root);
        SCOPED_TRACE(ent->d_name);
        regressionTest(source, sink);
    }
    closedir(dir);
}
