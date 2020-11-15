
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

/* Data Structure for Customers */


void *startServing(void *param);
typedef struct customers {

    int id;
    int class;
    float arrival;
    float service;
    int clerkID;
}   customer;   //Data structure holding the details of given customers.

// Global Variables
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define details 4  //Stores the 4 different details about the customer
#define max_input_size 1024    //Stores the number of customers, defaul 1024.
customer custList[max_input_size];  //An array made of data structures 'customers' that holds all the data structures.
int clerks[4] ={0, 0, 0, 0};    //0: Available 1: Busy
double overall_wait_time = 0;
double econ_wait_time = 0;
double bizz_wait_time = 0;
double init_time = 0;
int clerkId;
int clerksAvail = 4;
int singqlen = 0;
int econlen = 0;
int bizlen = 0;
int num_of_econ = 0;
int num_of_bizz = 0;
struct timeval start;
customer* singQ[max_input_size];
pthread_t tid;
pthread_mutex_t mutex;
pthread_cond_t convar;
pthread_attr_t attr;
pthread_t customer_tids[max_input_size];
// Global Variables end

/*
    fileName: name of the file
    info: 2D array which holds the file details
    Store file details into info array

*/

/*
    Adding customers to one single queue
    I tried multiple queues but this is easier.
*/
void addtoQueues(customer* cust){

    singQ[singqlen] = cust;
    singqlen++;

    if(cust->class == 0 ){
        econlen++;
    }
    else{
        bizlen++;
    }


}

/*
    Returns current system time
*/
double get_time(){

    struct timeval time;
    gettimeofday(&time, NULL);
    double seconds = (double)(time.tv_sec);
    double useconds = (double)(time.tv_usec)/1000000;
    // printf("%.2f\n", seconds+useconds);
    return seconds + useconds;

}

/*
    Reading the provided file
*/
int readFile(char *fileName, char info[max_input_size][max_input_size]){

    FILE *fp = fopen(fileName, "r");
    if( fp ){
        int i =0;
        while(fgets(info[i], max_input_size, fp)){
            i++;
        }
        fclose(fp);
        return 1;
    }
    else{
        return 0;
    }

}

/*
    Removing the customer from the queue when done servicing.
*/
void removesingq(customer* cust){
    
    
    int i =0;
    while( i < singqlen - 1){
        singQ[i] = singQ[i+1];
        i++;
    }
    singqlen--;


    if(cust->class == 0){
        econlen--;
    }
    else{
        bizlen--;
    }
}

/*
    Remove customers from queue when served.
*/
void releaseCustomer( customer* cust){
    
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&convar);
    removesingq(cust);
    pthread_mutex_unlock(&mutex);

}

/*
    Sorting to get business customers moving forward in the queue!
*/
void sortSing(){

    int x;
    int y;
    int sI = 4 - clerksAvail;

    for( x = sI; x < singqlen; x++){
        for( y = sI; y < singqlen -1; y++){
            if(singQ[y]->class == 0 && singQ[y+1]->class == 1){
                // printf("Customer ID: %d moves ahead of Customer ID: %d. \n", singQ[y+1]->id , singQ[y]->id);

                customer* temp = singQ[y+1];
                singQ[y+1] = singQ[y];
                singQ[y] = temp;
            }
        }
    }
}

/*
    Checking if the customer is ready to be served or not.
    If returned 1, then customer waits!
*/
int notMyTurn(customer* cust){

    if(clerksAvail == 0){
        return 1;
    }
    else if(clerksAvail == 1 && singQ[3]->id != cust->id){
        return 1;
    }
    return 0;
}

/*
    Wait until a clerk is available and its your turn!
    When its the customer's turn to be served, they leave this function!
*/
int findClerk(customer* cust){

    if(pthread_mutex_lock(&mutex) != 0){
        printf("Error: Failed to lock mutex\n");
        exit(EXIT_FAILURE);
    }

    if(cust->class == 0){
        printf("A customer" ANSI_COLOR_YELLOW " enters" ANSI_COLOR_RESET" a queue:\t\tCustomer ID: %d\t\tQueue ID: %d\t\tQueue Lenght: %d\n", cust->id, cust->class, econlen+1);
    }
    else{
        printf("A customer" ANSI_COLOR_YELLOW " enters" ANSI_COLOR_RESET" a queue:\t\tCustomer ID: %d\t\tQueue ID: %d\t\tQueue Lenght: %d\n", cust->id, cust->class, bizlen+1);
    }

    
    addtoQueues(cust);
    sortSing();
    
    while(notMyTurn(cust))
    {
        pthread_cond_wait(&convar, &mutex);
    }


    if(singqlen == 0 ){
        printf("No more customers left to serve\n");
    }
    else {

        for(int i = 0; i< 4 ; i++)
            {
                if(clerks[i] == 0)
                {
                    clerks[i] = 1;
                    cust->clerkID = i;
                    pthread_mutex_unlock(&mutex);
                    return i;
                }
                else
                {
                    continue;
                }   
            }
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

/*
    Adds wait time for both business and economy queues.
*/
void addWaitTime(customer* cust, float wait){

    overall_wait_time += wait;
    if(cust->class == 0){
        econ_wait_time += wait;
    }
    else
    {
        bizz_wait_time += wait;
    }
}

/*
    This is where the customers are added to the queue to be serviced.
*/
void* startServing(void *custList){

    customer* cust = (customer*)custList;
    usleep(cust->arrival*10);

    double arrT = get_time() - init_time;

    printf("A customer" ANSI_COLOR_GREEN" arrives"ANSI_COLOR_RESET ":\t\t\tCustomer ID: %d\t\t"ANSI_COLOR_GREEN"Arrival Time: %.2f s"ANSI_COLOR_RESET"\n", cust->id, arrT);
    double arrivT = get_time() - init_time;

    int clerknum = -1;
    clerknum = findClerk(cust);
    clerksAvail--;
    double startT = get_time() - init_time;

    printf("A clerk" ANSI_COLOR_BLUE" starts serving" ANSI_COLOR_RESET" a customer:\tCustomer ID: %d\t\t"ANSI_COLOR_BLUE"Start Time: %.2f s"ANSI_COLOR_RESET"\tClerk ID: %d\n", cust->id, startT, cust->clerkID+1);
    double waitT = startT - arrivT;
    addWaitTime(cust, waitT);
    usleep((cust->service)*10);
    clerksAvail++;
    double finishT = get_time() - init_time;
    printf("A clerk"ANSI_COLOR_RED" finishes serving"ANSI_COLOR_RESET" a customer:\tCustomer ID: %d\t\t"ANSI_COLOR_RED"Finish Time: %.2f s"ANSI_COLOR_RESET"\tClerk ID: %d\n", cust->id, finishT, cust->clerkID+1);
    clerks[cust->clerkID] = 0;

    releaseCustomer(cust);
    pthread_exit(NULL);
}

/*
    Function to check for any illegal values provided in the input file.
*/
void checkValues(customer cust){


    if(cust.arrival < 0){
        printf("Invalid Customer Arrival Time: %.2f s for Customer ID: %d\n\n", cust.arrival, cust.id);
        exit(EXIT_FAILURE);
    }
    else if(cust.service < 0){
        printf("Invalid Customer Service Time: %.2f s for Customer ID: %d\n\n", cust.service, cust.id);
        exit(EXIT_FAILURE);
    }
    else if(cust.id < 0){
        printf("Invalid Customer ID: %d\n\n", cust.id);
        exit(EXIT_FAILURE);
    }
    else if(cust.class >1 || cust.class<0){
        printf("Invalid Customer Class: %d for Customer ID: %d\n\n", cust.class, cust.id);
        exit(EXIT_FAILURE);
    }

}

int main(int argc, char* argv[]){

    printf("\n");
    if(argc < 2){
        printf(ANSI_COLOR_RED"Error:\t\tCorrect Usage: ./ACS <filename.txt>\n\n"ANSI_COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    char info[max_input_size][max_input_size];

    if(!readFile(argv[1], info)){
        printf("Error: File could not be read\n\n");
        exit(EXIT_FAILURE);
    }

    int numOfCustomers = atoi(info[0]);

/*
    Replacing the semi-colon to comas for consistency for further tokenizing.
*/
    for(int i =1; i <= numOfCustomers; i++){
        for(int j = 0; j < max_input_size; j++){
            if(info[i][j] == ':'){
                info[i][j] = ',';
            }
        }
    }


    for(int i =1; i<= numOfCustomers; i++){
        int j =0;
        int custDetails[details];
        char* token = strtok(info[i], ",");

        while (token != NULL)
        {
            custDetails[j] = atoi(token);
            token = strtok(NULL, ",");
            j++;
        }

        customer cust = { custDetails[0], custDetails[1], custDetails[2]*10000, custDetails[3]*10000, -1};  //A data structure that holds the details of all customers
        custList[i-1] = cust;   
        }
        //  At this point custList[i] would hold the information of ith customer from the text file.

        for(int i =0; i< numOfCustomers; i++){

            checkValues(custList[i]);
            
            customer display = custList[i];
            if(display.class == 0){
                num_of_econ++;
            }
            else{
                num_of_bizz++;
            }
        }

        // create a thread for each customer

        if(pthread_mutex_init(&mutex, NULL) != 0){
            printf("Error: Failed to initialize mutex\n");
            exit(EXIT_FAILURE);
        }
        if(pthread_cond_init(&convar, NULL) != 0){
            printf("Error: Failed to initialize conditional variable\n");
            exit(EXIT_FAILURE);
        }

        if(pthread_attr_init(&attr) != 0){
            printf("Error: Failed to initialize attribute\n");
            exit(EXIT_FAILURE);
        }

        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0){
            printf("Error: Failed to initialize detachstate\n");
        }

        init_time = get_time();

        for(int i = 0; i<numOfCustomers; i++){
            pthread_create(&customer_tids[i], &attr, startServing, (void*)&custList[i]) ;
        }


        for(int i =0; i < numOfCustomers; i++){
            pthread_join(customer_tids[i],NULL);
        }

        printf("The average waiting time for"ANSI_COLOR_YELLOW " all customers" ANSI_COLOR_RESET" in the system is:" ANSI_COLOR_YELLOW" %.2f seconds"ANSI_COLOR_RESET". \n", overall_wait_time/numOfCustomers);
        printf("The average waiting time for all" ANSI_COLOR_MAGENTA" business-class customers"ANSI_COLOR_RESET" in the system is:"ANSI_COLOR_MAGENTA" %.2f seconds"ANSI_COLOR_RESET" .\n", bizz_wait_time/num_of_bizz);
        printf("The average waiting time for all"ANSI_COLOR_CYAN" economy-class customers"ANSI_COLOR_RESET" in the system is:"ANSI_COLOR_CYAN" %.2f seconds"ANSI_COLOR_RESET" .\n", econ_wait_time/num_of_econ);
        

        pthread_attr_destroy(&attr);
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&convar);
        pthread_exit(NULL);
        return 0;

}