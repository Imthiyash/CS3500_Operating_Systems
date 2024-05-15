#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

int pid = 1;
map<int,int> address_map;

class file{
    public:
    string file_name;
    int process_id;
    int num_pages;
    int in_main_mem;
    vector<int> PTE;
    vector<string> commands;
    map<int,int> addr_values;
    file(string s, int a, int b, int c, vector<int> d, vector<string> p){
        file_name = s;
        process_id = a;
        num_pages = b;
        in_main_mem = c;
        PTE = d;
        commands = p;
    }
};

class vsort{
    public:
    bool operator() (file a, file b){
        return a.process_id < b.process_id;
    }
};

int translate_address(int virtual_address, int page_size, vector<int> PTE){
    int n = virtual_address/page_size;
    int offset = virtual_address - (page_size * n);
    if(n >= PTE.size()){
        return -1;
    }
    int physical_address = (PTE[n] * page_size) + offset;
    return physical_address;
}

void swapout(int& pid, vector<file>& pid_vector, int& main_pages, int& virtual_pages, vector<int>& main_memory, vector<int>& virtual_memory, ofstream& output_file){
    int found = 1;
    for(int i=0;i<pid_vector.size();i++){
        if(pid_vector[i].process_id == pid){
            if(pid_vector[i].in_main_mem == 1){
                if(pid_vector[i].num_pages <= virtual_pages){
                    for(auto p: pid_vector[i].addr_values){
                        address_map.erase(p.first);
                    }
                    main_pages += pid_vector[i].num_pages;
                    for(int j=0; j < pid_vector[i].PTE.size() ;j++){
                        main_memory[pid_vector[i].PTE[j]]--;
                    }
                    int num = pid_vector[i].num_pages;
                    virtual_pages -= num;
                    vector<int> v;
                    for(int i = 0;i<virtual_memory.size() && num!=0;i++){
                        if(virtual_memory[i]==0){
                            virtual_memory[i]++;
                            v.push_back(i);
                            num--;
                        }
                    }
                    pid_vector[i].PTE = v;
                    pid_vector[i].in_main_mem = 0;
                }
                else{
                    output_file<<"Swapout: Failed no sufficient space in virtual memory" << endl;
                }
            }
            found = 0;
            break;
        }
    }
    if(found){
        output_file <<"Swapout: Failed no file with given PID" << endl;
    }
}

void swapin(int& pid, vector<file>& pid_vector, int& main_pages, int& virtual_pages, vector<int>& main_memory, vector<int>& virtual_memory, ofstream& output_file){
    int found = 1;
    for(int i=0;i<pid_vector.size();i++){
        if(pid_vector[i].process_id == pid){
            if(pid_vector[i].in_main_mem == 0){
                if(pid_vector[i].num_pages <= main_pages){
                    virtual_pages += pid_vector[i].num_pages;
                    for(int j=0; j < pid_vector[i].PTE.size() ;j++){
                        virtual_memory[pid_vector[i].PTE[j]]--;
                    }
                    int num = pid_vector[i].num_pages;
                    main_pages -= num;
                    vector<int> v;
                    for(int i = 0;i<main_memory.size() && num!=0;i++){
                        if(main_memory[i]==0){
                            main_memory[i]++;
                            v.push_back(i);
                            num--;
                        }
                    }
                    pid_vector[i].PTE = v;
                    pid_vector[i].in_main_mem = 1;
                }
                else{
                    output_file<<"Swapin: Failed no sufficient space in main memory" << endl;
                }
            }
            found = 0;
            break;
        }
    }
    if(found){
        output_file<<"Swapin: Failed no file with given PID" << endl;
    }
}

void execute_commands(file& f, int p, ofstream& output_file){
    output_file << "Running file with PID: "<< f.process_id << endl;
    vector<string> s = f.commands;
    for(string line: s){
        istringstream iss(line);
        string command;
        iss >> command;
        if(command == "load"){
            string a1,b1;
            iss >> a1;
            iss >> b1;
            int a = stoi(a1.substr(0,(a1.length()-1)));
            int b = stoi(b1.substr(0,(b1.length())));
            int pb = translate_address(b,p,f.PTE);
            if(pb == -1){
                output_file << "Load: Failed invalid memory address "<< b <<" specified for PID " << f.process_id << endl;
                break;
            }
            else{
                address_map[pb] = a;
                f.addr_values[pb]++;
                string cout_o = "Command: load "+to_string(a)+" "+to_string(b)+"; "+"Result: Value of "+to_string(a)+" is now stored in addr "+to_string(b);
                output_file << cout_o << endl;
            }
        }
        else if(command == "add"){
            string a1,b1,c1;
            iss >> a1;
            iss >> b1;
            iss >> c1;
            int a = stoi(a1.substr(0,(a1.length()-1)));
            int b = stoi(b1.substr(0,(b1.length()-1)));
            int c = stoi(c1.substr(0,(c1.length())));
            int pa = translate_address(a,p,f.PTE);
            int pb = translate_address(b,p,f.PTE);
            int pc = translate_address(c,p,f.PTE);
            if(pa == -1){
                output_file << "Add: Failed invalid memory address "<< a <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else if(pb == -1){
                output_file << "Add: Failed invalid memory address "<< b <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else if(pc == -1){
                output_file << "Add: Failed invalid memory address "<< c <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else{
                f.addr_values[pa]++;
                f.addr_values[pb]++;
                f.addr_values[pc]++;
                address_map[pc] = address_map[pa] + address_map[pb];
                string cout_o = "Command: add "+to_string(a)+","+to_string(b)+","+to_string(c)+"; "+"Result: Value in addr "+to_string(a)+" = "+to_string(address_map[pa])+", addr "+to_string(b)+" = "+to_string(address_map[pb])+", addr "+to_string(c)+" = "+to_string(address_map[pc]);
                output_file << cout_o << endl;
            }
        }
        else if(command == "sub"){
            string a1,b1,c1;
            iss >> a1;
            iss >> b1;
            iss >> c1;
            int a = stoi(a1.substr(0,(a1.length()-1)));
            int b = stoi(b1.substr(0,(b1.length()-1)));
            int c = stoi(c1.substr(0,(c1.length())));
            int pa = translate_address(a,p,f.PTE);
            int pb = translate_address(b,p,f.PTE);
            int pc = translate_address(c,p,f.PTE);
            if(pa == -1){
                output_file << "Add: Failed invalid memory address "<< a <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else if(pb == -1){
                output_file << "Add: Failed invalid memory address "<< b <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else if(pc == -1){
                output_file << "Add: Failed invalid memory address "<< c <<" specified for PID " << f.process_id<< endl;
                break;
            }
            else{
                f.addr_values[pa]++;
                f.addr_values[pb]++;
                f.addr_values[pc]++;
                address_map[pc] = address_map[pa] - address_map[pb];
                string cout_o = "Command: add "+to_string(a)+","+to_string(b)+","+to_string(c)+"; "+"Result: Value in addr "+to_string(a)+" = "+to_string(address_map[pa])+", addr "+to_string(b)+" = "+to_string(address_map[pb])+", addr "+to_string(c)+" = "+to_string(address_map[pc]);
                output_file << cout_o << endl;
            }
        }
        else if(command == "print"){
            int a;
            iss >> a;
            int pa = translate_address(a,p,f.PTE);
            if(pa == -1){
                output_file << "Print: Failed invalid memory address "<< a <<" specified for PID " << f.process_id << endl;
                break;
            }
            else{
                f.addr_values[pa]++;
                string cout_o = "Command: print " + to_string(a) + "; "+"Result: Value in addr "+to_string(a)+" = "+to_string(address_map[pa]);
                output_file << cout_o << endl;
            }
        }
    }
}

int main(int argc, char* argv[]){
    int M,V,P;
    string S,O;
    M = stoi(argv[2]);
    V = stoi(argv[4]);
    P = stoi(argv[6]);
    S = argv[8];
    O = argv[10];
    float page_size = P/1024.0;
    int main_pages = M/page_size;
    int virtual_pages = V/page_size;
    vector<int> main_memory(main_pages);
    vector<int> virtual_memory(virtual_pages);
    vector<int> curr_processes;
    vector<int> main_processes;
    ifstream input_file(S);
    ofstream output_file(O);
    vector<file> pid_vector;
    string line;
    while(getline(input_file, line)){
        istringstream iss(line);
        string command;
        iss >> command;
        if(command == "load"){
            string file_name;
            while(iss >> file_name){
                ifstream new_file(file_name);
                string new_line;
                int num = 0;
                if(getline(new_file, new_line)){
                    num = stoi(new_line);
                }
                num = (num/page_size);
                vector<string> commands;
                while(getline(new_file, new_line)){
                    commands.push_back(new_line);
                }
                if(num <= main_pages){
                    main_pages -= num;
                    vector<int> v;
                    for(int i = 0;i<main_memory.size() && num!=0;i++){
                        if(main_memory[i]==0){
                            main_memory[i]++;
                            v.push_back(i);
                            num--;
                        }
                    }
                    file f(file_name, pid, v.size(), 1, v, commands);
                    pid_vector.push_back(f);
                    main_processes.push_back(pid);
                    string cout_o = file_name+" is loaded in main memory and is assigned process id "+ to_string(pid);
                    output_file << cout_o << endl;
                    pid++;
                }
                else if(num <= virtual_pages){
                    virtual_pages -= num;
                    vector<int> v;
                    for(int i = 0;i<virtual_memory.size() && num!=0;i++){
                        if(virtual_memory[i]==0){
                            virtual_memory[i]++;
                            v.push_back(i);
                            num--;
                        }
                    }
                    file f(file_name, pid, v.size(), 0, v, commands);
                    pid_vector.push_back(f);
                    string cout_o = file_name+" is loaded in virtual memory and is assigned process id "+to_string(pid);
                    output_file << cout_o << endl;
                    pid++;
                }
                else{
                    string cout_o  = file_name+" could not be loaded - memory is full";
                    output_file << cout_o << endl;
                }
                new_file.close();
            }
        }
        else if(command == "run"){
            int num = 0;
            iss >> num;
            int no_run = 1;
            for(int i=0;i<pid_vector.size();i++){
                if(pid_vector[i].process_id == num){
                    if(pid_vector[i].in_main_mem == 1){
                        execute_commands(pid_vector[i], (page_size*1024), output_file);
                        if(curr_processes.size()==0){
                            curr_processes.push_back(num);
                        }
                        for(int j=0;j<curr_processes.size();j++){
                            if(curr_processes[j]==num){
                                curr_processes.erase(curr_processes.begin()+j);
                                curr_processes.push_back(num);
                                break;
                            }
                        }
                        output_file<<"Run: command successful"<<endl;
                        no_run = 0;
                        break;
                    }
                    else{
                        file f = pid_vector[i];
                        sort(main_processes.begin(),main_processes.end());
                        while(f.num_pages > main_pages){
                            if(curr_processes.size()!=0){
                                swapout(curr_processes[0], pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                                for(int i=0;i<main_processes.size();i++){
                                    if(main_processes[i] == curr_processes[0]){
                                        main_processes.erase(main_processes.begin()+i);
                                        break;
                                    }
                                }
                                curr_processes.erase(curr_processes.begin()+0);
                            }
                            else{
                                swapout(main_processes[0], pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                                curr_processes.erase(main_processes.begin()+0);
                            }
                        }
                        swapin(num, pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                        curr_processes.push_back(num);
                        execute_commands(f, (page_size*1024), output_file);
                        output_file << "Run: command successful" << endl;
                        no_run = 0;
                        break;
                    }
                }
            }
            if(no_run){
                output_file<<"Run: Failed no file with given PID" << endl;
            }
        }
        else if(command == "kill"){
            int num = 0;
            iss >> num;
            int i = 0;
            int no_kill = 1;
            for(i=0;i<pid_vector.size();i++){
                if(pid_vector[i].process_id == num){
                    for(auto p: pid_vector[i].addr_values){
                        address_map.erase(p.first);
                    }
                    pid_vector[i].addr_values.clear();
                    no_kill = 0;
                    if(pid_vector[i].in_main_mem == 1){
                        for(int j=0; j < pid_vector[i].PTE.size() ;j++){
                            main_memory[pid_vector[i].PTE[j]]--;
                        }
                        for(int i=0;i<main_processes.size();i++){
                            if(main_processes[i] == num){
                                main_processes.erase(main_processes.begin()+i);
                                break;
                            }
                        }
                        for(int i=0;i<curr_processes.size();i++){
                            if(curr_processes[i] == num){
                                curr_processes.erase(curr_processes.begin()+i);
                                break;
                            }
                        }
                    }
                    else{
                        for(int j=0; j < pid_vector[i].PTE.size() ;j++){
                            virtual_memory[pid_vector[i].PTE[j]]--;
                        }
                    }
                    pid_vector.erase(pid_vector.begin()+i);
                    output_file<<"Kill: command successful" << endl;
                    break;
                }
            }
            if(no_kill){
                output_file<<"Kill: Failed no file with given PID" << endl;
            }
        }
        else if(command == "listpr"){
            vector<int> mpid,vpid;
            for(int i=0;i<pid_vector.size();i++){
                if(pid_vector[i].in_main_mem == 1){
                    mpid.push_back(pid_vector[i].process_id);
                }
                else{
                    vpid.push_back(pid_vector[i].process_id);
                }
            }
            sort(mpid.begin(),mpid.end());
            sort(vpid.begin(),vpid.end());
            string cout_o = "Main memory processes: ";
            for(int i:mpid){
                cout_o = cout_o + to_string(i) + " ";
            }
            output_file << cout_o << endl;
            cout_o = "Virtual memory processes: ";
            for(int i: vpid){
                cout_o = cout_o + to_string(i) + " ";
            }
            output_file << cout_o << endl;
        }
        else if(command == "pte"){
            int a;string s;
            iss >> a;
            iss >> s;
            ofstream output2(s,std::ios::app);
            time_t r = time(0);
            char* time_date = ctime(&r);
            output2 << time_date << endl;
            int no_pid = 1;
            for(int i=0;i<pid_vector.size();i++){
                if(pid_vector[i].process_id == a){
                    output2<<"logical p.no\tphysical p.no"<< endl;
                    for(int j=0;j<pid_vector[i].PTE.size();j++){
                        string cout_o = to_string(j)+"\t\t"+to_string(pid_vector[i].PTE[j]);
                        output2 << cout_o << endl;
                    }
                    no_pid = 0;
                    break;
                }
            }
            if(no_pid){
                output_file<<"PTE: Failed no file with given PID"<<endl;
            }
        }
        else if(command == "pteall"){
            string s;
            iss >> s;
            vector<file> v = pid_vector;
            sort(v.begin(),v.end(),vsort());
            ofstream output2(s,std::ios::app);
            time_t r = time(0);
            char* time_date = ctime(&r);
            output2 << time_date << endl;
            for(int i=0;i<v.size();i++){
                string cout_o = "For process with PID: " + to_string(v[i].process_id);
                output2 << cout_o << endl;
                output2 <<"logical p.no\tphysical p.no"<< endl;
                for(int j=0;j<v[i].PTE.size();j++){
                    string cout_o = to_string(j)+"\t\t"+to_string(v[i].PTE[j]);
                    output2 << cout_o << endl;
                }
            }
        }
        else if(command == "swapout"){
            int s;
            iss >> s;
            for(int j=0;j<curr_processes.size();j++){
                if(curr_processes[j]==s){
                    curr_processes.erase(curr_processes.begin()+j);
                    break;
                }
            }
            swapout(s, pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
            for(int i=0;i<main_processes.size();i++){
                if(main_processes[i] == s){
                    main_processes.erase(main_processes.begin()+i);
                    break;
                }
            }
        }
        else if(command == "swapin"){
            int s;
            iss >> s;
            int not_found = 1;
            sort(main_processes.begin(),main_processes.end());
            for(int i=0;i<pid_vector.size();i++){
                if(pid_vector[i].process_id == s){
                    if(pid_vector[i].in_main_mem == 0){
                        while(pid_vector[i].num_pages > main_pages){
                            if(curr_processes.size()!= 0){
                                swapout(curr_processes[0], pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                                string cout_o = "Process with PID " + to_string(curr_processes[0]) + " swapped out";
                                output_file << cout_o << endl;
                                for(int i=0;i<main_processes.size();i++){
                                    if(main_processes[i] == curr_processes[0]){
                                        main_processes.erase(main_processes.begin()+i);
                                        break;
                                    }
                                }
                                curr_processes.erase(curr_processes.begin()+0);
                            }
                            else{
                                if(main_processes.size()!=0){
                                    swapout(main_processes[0], pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                                    string cout_o = "Process with PID " + to_string(main_processes[0]) + " swapped out";
                                    output_file << cout_o << endl;
                                    main_processes.erase(main_processes.begin()+0);
                                }
                                else{
                                    break;
                                }
                            }
                        }
                        if(pid_vector[i].num_pages > main_pages){
                            swapin(s, pid_vector, main_pages, virtual_pages, main_memory, virtual_memory, output_file);
                            main_processes.push_back(s);
                        }
                        else{
                            output_file << "Swapin: Failed file cannot be placed in main memory" << endl;
                        }
                    }
                    not_found = 0;
                    break;
                }
            }
            if(not_found){
                output_file << "Swapin: Failed no file with given PID" << endl;
            }
        }
        else if(command == "print"){
            int a,b;
            iss >> a;
            iss >> b;
            for(int i=0;i<b;i++){
                string cout_o = "Value of "+to_string(a+i)+": "+to_string(address_map[a+i]);
                output_file << cout_o << endl;
            }
        }
        else if(command == "exit"){
            for(int &i: main_memory)i = 0;
            for(int &i: virtual_memory)i = 0;
            input_file.close();
            exit(0);
        }
    }
    return 0;
}