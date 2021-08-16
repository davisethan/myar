#include "myar.h"

int main(int argc, char **argv){
    if(argc < 3){ // Error handling
        fprintf(stderr, "Error: Usage \"myar -qxtvdA archive-file file...\"\n");
        exit(EXIT_FAILURE);
    }

    if(shouldAppend(argv)){ // -q
        doAppend(argc, argv);
    }else if(shouldExtract(argv)){ // -x
        doExtract(argc, argv);
    }else if(shouldPrintConciseTable(argv)){ // -t
        doPrintConciseTable(argc, argv);
    }else if(shouldPrintVerboseTable(argv)){ // -v
        doPrintVerboseTable(argc, argv);
    }else if(shouldDelete(argv)){ // -d
        doDelete(argc, argv);
    }else if(shouldAppendAll(argv)){ // -A
        doAppendAll(argc, argv);
    }else{
        fprintf(stderr, "Error: Usage \"myar -qxtvdA archive-file file...\"\n");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}
