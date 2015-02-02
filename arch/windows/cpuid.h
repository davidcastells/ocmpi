//NUM_OF_PROCESSORS

#define RETURN_CPU_IDENTIFIER() return getCpuIdentifier()


typedef int (*main_func) (int argc, char* args[]);

int getCpuIdentifier();
void createSlaveThreads(main_func mf);