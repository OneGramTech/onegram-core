//
// Created by Lubos Ilcik on 27/01/2018.
//

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
   framework::master_test_suite().p_name.value = "Fee Schedule Test Suite";
   return nullptr;
}
