#include <unistd.h>
#include <iostream>

using namespace std;

#include "GPR.h"

int main(int argc, char **argv) {

  if(getuid()) {
    printf("This program needs to be run as root.\n");
    exit (1);
  }

  cout << "Starting..." << endl;

  GPR *gpr = GPR::getInstance();

  return 0;
}
