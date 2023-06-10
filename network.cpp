#include <iostream>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
using namespace std;

#define input 2
#define layers 5
int numlayer = 0;
#define neurons 8
#define output 1
#define loops 2

int* num, neuralloop;
pthread_mutex_t mut;

struct neuron{
    double inp;
    int ind;
};

void* neurthread (void* data){
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    neuron temp = *(neuron*)data;
    int ind = temp.ind;
    double inpval = 0;

    double* weights = new double[neurons];

    if (numlayer == 0){
        inpval = temp.inp;
    }
    else{
        int loopcount;
        if (numlayer > 1){
            loopcount = neurons;  
        }
        else{
            loopcount = input;
        }
        for (int i = 0; i < loopcount; i += 1){
            string str = "pipe";
            str += to_string(numlayer - 1);
            str += to_string(i);
            str += to_string(ind);

            double* temp = new double;
            mkfifo(str.c_str(), 0666);
            int fd = open(str.c_str(), O_RDONLY);
            read(fd, temp, sizeof(*temp));
            inpval += *temp;  
            close(fd);     
            unlink(str.c_str());           
        }
    }

    ifstream mfile("config.txt");
    string str, comp = "";

    if (numlayer != layers + 1){
        if (numlayer != 0){             
            comp += "Hidden layer ";
            comp += to_string(numlayer);
            comp += " weights";
        }
        else{
            comp = "Input layer weights";
        }

        while (getline(mfile, str)){
            if (str == comp){
                for (int i = 0; i < ind + 1; i++){
                    getline(mfile, str);
                }
                string valtemp = "";
                int x = 0;

                for (int i = 0; i < str.length(); i++){
                    if (str[i] == ',' || i == str.length()-1 || str[i] == ' ' && str[i+1] == ' '){
                        if (str[i] != ',' && str[i] != ' '){
                            valtemp += str[i];
                        }
                        weights[x] = stod(valtemp);
                        valtemp = "";
                        x++;
                        if (str[i] == ' ' && str[i+1] == ' '){
                            break;
                        }
                    }
                    else{
                        if (str[i] != ' '){
                            valtemp += str[i];
                        }
                    }
                }
            }
        }
        mfile.close();

        for (int i = 0; i < neurons; i++){
            weights[i] = inpval * weights[i];
        }
        
        //writing for the next layer
        int loopcount;
        if (numlayer == layers){
            loopcount = output;
        }
        else{
            loopcount = neurons;
        }
        int* fd = new int[loopcount];
        string* str = new string[loopcount];
        for (int i = 0; i < loopcount; i += 1){
            str[i] = "pipe";
            str[i] += to_string(numlayer);
            str[i] += to_string(ind);
            str[i] += to_string(i);
            //double temp = weights[i];
            fd[i] = open(str[i].c_str(), O_WRONLY | O_CREAT, 0666);

            write(fd[i], &weights[i], sizeof(weights[i]));    
            close(fd[i]);  
            usleep(100000);   
        }
    }
    else{
        //Output layer
        //put input in equations, display, and send back
        double* fx = new double[2];
        fx[0] = ((inpval*inpval)+inpval+1)/2;
        cout << "fx1 is: " << fx[0] << endl;
        fx[1] = ((inpval*inpval)-inpval)/2;
        cout << "fx2 is: " << fx[1] << endl;

        if (neuralloop < loops - 1){
            for (int i = 0; i < 2; i += 1){
                string str = "backed";
                str += to_string(numlayer);
                str += to_string(i);
                
                int fd = open(str.c_str(), O_WRONLY | O_CREAT, 0666);
                write(fd, &fx[i], sizeof(fx[i]));
                close(fd);        
            }
        }
    }
    *num -= 1;
    pthread_exit(NULL);
}



int main(){
    string str;
    ifstream mfile("config.txt");
    double weights[input][neurons];  
    neuron first[input];  
    double inpval[input];

    while (getline(mfile, str)){
        if (str == "Input data"){
            getline(mfile, str);
            string valtemp = "";
            int x = 0;
            for (int i = 0; i < str.length(); i++){
                if (str[i] == ',' || i == str.length()-1){
                    if (str[i] != ','){
                        valtemp += str[i];
                    }
                    first[x].inp = stod(valtemp);
                    first[x].ind = x+1;
                    valtemp = "";
                    x++;
                }
                else{
                    if (str[i] != ' '){
                        valtemp += str[i];
                    }                   
                }
            
            }
            mfile.close();
            break;
        }
    }
       
    //this creates the layers
    for (neuralloop = 0; neuralloop < loops; neuralloop++){
        numlayer = 0;
        cout << "\nRun Number " << neuralloop+1 << endl;
        for (int x = 0; x < input; x++){
            cout << "Input " << x + 1 << " value is: " << first[x].inp << endl;
        }
        for (int i = 0; i <= layers + 1; i++){
            if (!fork()){
                num = new int;
                *num = 0;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                pthread_mutex_init(&mut, NULL);

                if (numlayer == 0){
                    pthread_t pid[input];
                    for (int x = 0; x < input; x++){
                        neuron* temp = new neuron;
                        temp->inp = first[x].inp;
                        temp->ind = x;
                        pthread_create(&pid[x], &attr, neurthread, temp);
                        *num += 1;
                    }
                    while (*num > 0){
                        usleep(500000);
                    }
                }
                else if(numlayer == layers + 1){
                    cout << "--------OUTPUT--------" << endl;
                    pthread_t pid[output];
                    for (int x = 0; x < output; x++){
                        neuron* temp = new neuron;
                        temp->ind = x;
                        pthread_create(&pid[x], &attr, neurthread, temp);
                        *num += 1;
                    }
                    while (*num > 0){
                        usleep(500000);
                    }
                    exit(0);
                }
                else {
                    pthread_t pid[neurons];
                    for (int x = 0; x < neurons; x++){
                        neuron* temp = new neuron;
                        temp->ind = x;
                        pthread_create(&pid[x], &attr, neurthread, temp);
                        *num += 1;
                    }
                    while (*num > 0){
                        usleep(500000);
                    }
                    
                }

                if (neuralloop < loops - 1){
                    double* back = new double[2];
                    
                    for (int i = 0; i < 2; i += 1){
                        string str = "backed";
                        str += to_string(numlayer + 1);
                        str += to_string(i);
                        mkfifo(str.c_str(), 0666);
                        int fd = open(str.c_str(), O_RDONLY);
                        read(fd, &back[i], sizeof(back[i]));
                        close(fd);                         
                        unlink(str.c_str());       
                    }
                    if (numlayer > 0){
                        cout << "\nlayer number: " << numlayer << endl;
                    }
                    else{
                        cout << "\nInput Layer: " << endl;
                    }
                    
                    cout << "fx1: " << back[0] << endl;
                    cout << "fx2: " << back[1] << endl;

                    if (numlayer > 0){
                        for (int i = 0; i < 2; i += 1){    
                            string str = "backed";   
                            str += to_string(numlayer);      
                            str += to_string(i);                              
                            int fd = open(str.c_str(), O_WRONLY | O_CREAT, 0666);
                            write(fd, &back[i], sizeof(back[i]));
                            close(fd);        
                        }
                    }
                    else{
                        for (int i = 0; i < 2; i += 1){    
                            string str = "backed";         
                            str += to_string(i);                                 
                            int fd = open(str.c_str(), O_WRONLY | O_CREAT, 0666);
                            write(fd, &back[i], sizeof(back[i]));
                            close(fd);        
                        }
                    }
                    
                }
                exit(0);
            }        
            numlayer += 1;
            
        }

        if (neuralloop < loops - 1){
            for (int i = 0; i < 2; i += 1){
                string str = "backed";
                str += to_string(i);

                mkfifo(str.c_str(), 0666);
                int fd = open(str.c_str(), O_RDONLY);
                read(fd, &first[i].inp, sizeof(first[i].inp));
                close(fd);                
                unlink(str.c_str());      
            }
        }
        int status = 0;
        pid_t pid;
        while ((pid = wait(&status)) > 0);
    }
}