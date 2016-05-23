
#include <stdio.h>
#include <stdlib.h>
#include <ab_param.h>
#include <ab_test.h>
#include <ab_log.h>


void use_format() {
    printf("ab [options] http://hostname[:port][/path]\n"
           "    -n requests number of requests to perform\n"
           "    -c concurrency number of multiple requests to make\n"
           "    -t timelimit seconds to max. wait for responses\n"
           "    -i use HEAD instead of GET\n"
           "    -v verbose\n");
    exit(0);
}


int
main(int argc, char *argv[]) {

    char *err;

    if (argc < 2)
        use_format(); //

    if ((err = ab_param_parse(argc, argv)) != NULL) {
        printf("parse param error: %s\n", err);
        use_format();
    }

    ab_log("This is HTTP Bench Version 0.1");
    ab_param_dump();
    ab_test_start();
    ab_test_result();
    
    return 0;
}
