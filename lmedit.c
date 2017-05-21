///author: jmp1617
///purpose: l2k module editor
///Git was used for VCS
#include <stdio.h>
#include <stdlib.h>
#include "exec.h"
#include <arpa/inet.h>
#include <string.h>

///struct to represent a single command for history
typedef struct history_cmd{
    char* command;
    int seqnum;
}history_cmd_t;

///struct to represent entire module in memory
typedef struct module{
    exec_t* HEADER;
    uint8_t* TEXT;
    uint8_t* RDATA;
    uint8_t* DATA;
    uint8_t* SDATA;
    uint8_t* SBSS;
    uint8_t* BSS;
    relent_t* RELTAB;
    refent_t* REFTAB;
    syment_t* SYMTAB;
    uint8_t* STRINGS;
}module_t;

///create a bit mask to extract a certain field of bits
///param a start and b end point of mask
unsigned createMask(unsigned a, unsigned b){
    unsigned r = 0;
    for(unsigned i=a; i<=b; i++){
        r |= 1 << i;
    }
    return r;
}

///convert binary to decimal
///param: data to convert, lenbin the size of the binary
int btod(unsigned data, int lenbin){
    int result = 0;
    int vals[lenbin];
    for(int i=0;i<lenbin;i++){//generate the binary table
        if(i==0){
           vals[i]=1;
        }
        else{
           vals[i] = vals[i-1]*2;
        }
    }
    //create the initial mask
    unsigned mask = 1;
    for(int i=0;i<lenbin;i++){
        if((data & mask)>0){
            result+=vals[i];
        }
        mask = mask<<1;
    }
    return result;
}

///convert the binary version number to a human readable data
///param: version number to convert
void convertversion(uint16_t version){
    struct{//totals the 16 bit version
        unsigned year : 7;
        unsigned month : 4;
        unsigned day : 5;
    }ver;
    ver.day = createMask(0,4) & version;
    version = version>>5;
    ver.month = createMask(0,3) & version;
    version = version>>4;
    ver.year = createMask(0,7) & version;
    int year = btod(ver.year,7)+2000;
    int month = btod(ver.month,4);
    int day = btod(ver.day,5);
    if(month<10 && day<10){
        printf("%d/0%d/0%d",year,month,day);
    }
    else if(month<10){
        printf("%d/0%d/%d",year,month,day);
    }
    else if(day<10){
        printf("%d/%d/0%d",year,month,day);
    }
    else{
        printf("%d/%d/%d",year,month,day);
    }
}

///function to try and open the file and check if it is a module with the magic number
///param: file to open
FILE* open_module(char* file){
    FILE* fp = fopen(file, "r");//read now write later
    if(!fp){
        perror("error: file could not be opened");
        return NULL;
    }
    else{
        uint16_t magic;
        fread(&magic,sizeof(uint16_t),1,fp);
        if(ntohs(magic)!=HDR_MAGIC){
            fprintf(stderr,"error: %s is not an R2K object module (magic number 0x%x)\n",file,ntohs(magic));
            return NULL;
        }
        else{
            return fp;
        }
    }
}

///free the module
void destroy_module(module_t* MODULE){
    if(MODULE->HEADER){
        free(MODULE->HEADER);
    }
    if(MODULE->TEXT){
        free(MODULE->TEXT);
    }
    if(MODULE->RDATA){
        free(MODULE->RDATA);
    }
    if(MODULE->DATA){
        free(MODULE->DATA);
    }
    if(MODULE->SDATA){
        free(MODULE->SDATA);
    }
    if(MODULE->SBSS){
        free(MODULE->SBSS);
    }
    if(MODULE->BSS){
        free(MODULE->BSS);
    }
    if(MODULE->RELTAB){
        free(MODULE->RELTAB);
    }
    if(MODULE->REFTAB){
        free(MODULE->REFTAB);
    }
    if(MODULE->SYMTAB){
        free(MODULE->SYMTAB);
    }
    if(MODULE->STRINGS){
        free(MODULE->STRINGS);
    }
    free(MODULE);
}

void print_summary(exec_t* header, char* name){
    if(header->entry == 0x0){
        printf("File %s is an R2K object module\n",name);
    }
    else{
        printf("File %s is an R2K load module (entry point %#010x)\n",name,ntohl(header->entry));
    }
    printf("Module version: ");
    convertversion(ntohs(header->version));
    printf("\n");
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    for(int i=0;i<N_EH;i++){
        if(ntohl(header->data[i])!=0){
            if(i<6||i==9){
                printf("Section %s is %d bytes long\n",sections[i],ntohl(header->data[i]));
            }
            else{
                printf("Section %s is %d entries long\n",sections[i],ntohl(header->data[i]));
            }
        }
    }
}

///fuction to get the size of a section
///param: section section to get size, module current module
///return: size of section
int get_size(char* section, module_t* MODULE){
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    int size = 0;
    for(int sec=0;sec<N_EH;sec++){
        if(!strcmp(section,sections[sec])){
            size = ntohl(MODULE->HEADER->data[sec]);
            return size;
        }
    }
    return 0;
}

///fuction to update the history array
///param: cmd the command to add, history the history array
///,n the sequence num, size the number of commands in the history
void add_to_history(char* cmd, int n, history_cmd_t* history[], int* size){
    if(*size<10){//add the command to the array;
        history[*size] = malloc(sizeof(history_cmd_t));
        history[*size]->command = malloc(strlen(cmd)+1);
        strncpy(history[*size]->command,cmd,strlen(cmd)+1);
        history[*size]->seqnum = n;
        (*size)++;
    }
    else{//must push out the old
        free(history[0]->command);//free the oldest history
        free(history[0]);
        history[0]->command = NULL;
        history[0] = NULL;
        for(int i=1;i<10;i++){//shift every pointer over one index
            history[i-1] = history[i];
        }
        history[9] = malloc(sizeof(history_cmd_t));//create the newest entry
        history[9]->command = malloc(strlen(cmd)+1);
        strncpy(history[9]->command,cmd,strlen(cmd)+1);
        history[9]->seqnum = n;
    }
}

///routine to destroy the history
///param: history history array, size size of history
void destroy_history(history_cmd_t* history[], int* size){
    for(int entry=0;entry<*size;entry++){
        free(history[entry]->command);
        free(history[entry]);
    }
    (*size) = 0;
}

///routine to write the module to the file 
///param: MODULE module to write
void write(module_t* MODULE, char* filename){
    FILE* mfp = fopen(filename,"r+w");
    //position the fp at the beginning of the file
    fseek(mfp,52,SEEK_SET);//offset 52 bytes to skip the heade
    //write out the sections
    if(MODULE->HEADER->data[EH_IX_TEXT]){
        for(int byte=0;byte<get_size("text",MODULE);byte++){
            fwrite(&(MODULE->TEXT[byte]),1,1,mfp);
        }
    }
    if(MODULE->HEADER->data[EH_IX_RDATA]){
        for(int byte=0;byte<get_size("rdata",MODULE);byte++){
            fwrite(&(MODULE->RDATA[byte]),1,1,mfp);
        }
    }
    if(MODULE->HEADER->data[EH_IX_DATA]){
        for(int byte=0;byte<get_size("data",MODULE);byte++){
            fwrite(&(MODULE->DATA[byte]),1,1,mfp);
        }
    }
    if(MODULE->HEADER->data[EH_IX_SDATA]){
        for(int byte=0;byte<get_size("sdata",MODULE);byte++){
            fwrite(&(MODULE->SDATA[byte]),1,1,mfp);
        }
    }
    if(MODULE->HEADER->data[EH_IX_SBSS]){
        fseek(mfp,get_size("sbss",MODULE),SEEK_CUR);//sbss cant be changed
    }
    if(MODULE->HEADER->data[EH_IX_BSS]){
        fseek(mfp,get_size("bss",MODULE),SEEK_CUR);//bss cant be changed
    }
    if(MODULE->HEADER->data[EH_IX_REL]){
        for(int entry=0;entry<(get_size("reltab",MODULE));entry++){//for the num entries
            fseek(mfp,8,SEEK_CUR);//skip the size of a rel entry
        }
    } 
    if(MODULE->HEADER->data[EH_IX_REF]){
        for(int entry=0;entry<(get_size("reftab",MODULE));entry++){//for the num entries
            fseek(mfp,12,SEEK_CUR);//skip the size of a ref entry
        }   
    }
    if(MODULE->HEADER->data[EH_IX_SYM]){
        for(int entry=0;entry<(get_size("symtab",MODULE));entry++){//for the num entries
            fseek(mfp,12,SEEK_CUR);//skip the size of a sym entry
        }   
    }
    if(MODULE->HEADER->data[EH_IX_STR]){
        for(int byte=0;byte<get_size("strings",MODULE);byte++){
            fwrite(&(MODULE->STRINGS[byte]),1,1,mfp);
        }
    }
    fflush(mfp);
    fclose(mfp);
}

///fuction to see if a command contains a certain char
///param: command, check char to check for
///return: true or false
int check_char(char* command, char check){
    int contains = 0;
    for(unsigned int c=0;c<strlen(command);c++){
        if(command[c] == check){
            contains = 1;
        }
    }
    return contains;
}


///fuction to count chars
///param: char* string, char c to look for
///return count of that char
int count_char(char* word, char c){
    int count = 0;
    for(unsigned int i=0;i<strlen(word);i++){
        if(word[i]==c){
            count++;
        }
    }
    return count;
}

///function to analyze the buffer for examine command format
///command[4]:
///   1 = just address has been entered
///   2 = just type has been entered
///   3 = both have been entered
///param: command array to hold values, buf string to analyze
///return: 1 if not x command
int proccess_x_command(unsigned int command[4],char* buf){
    command[0]=0;
    command[1]=1;
    command[2]='w';
    command[3]=0;
    command[4]=0;//1 if just address, 2 iff adress and size
    unsigned int address=0,count=1,change=0;
    char type='w';
    if(!buf){
        return 1;
    }
    else{
        if(count_char(buf,'=')){
            command[4]++;
        }
        if(count_char(buf,':')){
            command[4]+=2;
        }
        if(command[4]!=1&&command[4]!=3){//if it is not a write command
            int numx = count_char(buf,'x');//see how many times a hex val was used
            if(numx){//hex was used for the address
                if(command[4]==2){//if type was used but no write
                    if(sscanf(buf,"0x%x:%c",&address,&type)==2){
                        command[0]=address;
                        command[2]=type;
                        return 0;
                    }
                    else if(sscanf(buf,"0x%x,%u:%c",&address,&count,&type)==3){
                        command[0]=address;
                        command[1]=count;
                        command[2]=type;
                        return 0;
                    }
                }
                else{//there is not type specified or write
                    if(sscanf(buf,"0x%x,%u",&address,&count)>=1){
                        command[0]=address;
                        command[1]=count;
                        return 0;
                    }
                }
            }
            else{//no hex was used
                if(command[4]>=2){//if type was used but no write
                    if(sscanf(buf,"%u:%c",&address,&type)==2){
                        command[0]=address;
                        command[2]=type;
                        return 0;
                    }
                    else if(sscanf(buf,"%u,%u:%c",&address,&count,&type)==3){
                        command[0]=address;
                        command[1]=count;
                        command[2]=type;
                        return 0;
                    }
                }
                else{//there is not type specified or write
                    if(sscanf(buf,"%u,%u",&address,&count)>=1){
                        command[0]=address;
                        command[1]=count;
                        return 0;
                    }
                }
            }
        }
        else{//there is a write command
            int numx = count_char(buf,'x');
            if(numx==2){//both the address and the new data are hex
                if(command[4]==3){//the type spec was used
                    if(sscanf(buf,"0x%x:%c=0x%x",&address,&type,&change)==3){
                        command[0]=address;
                        command[2]=type;
                        command[3]=change;
                        return 0;
                    }
                    else if(sscanf(buf,"0x%x,%u:%c=0x%x",&address,&count,&type,&change)==4){
                        command[0]=address;
                        command[1]=count;
                        command[2]=type;
                        command[3]=change;
                        return 0;
                    }
                }
                else{//the type spec was not used
                    if(sscanf(buf,"0x%x=0x%x",&address,&change)==2){
                        command[0]=address;
                        command[3]=change;
                        return 0;
                    }
                    else if(sscanf(buf,"0x%x,%u=0x%x",&address,&count,&change)){
                        command[0]=address;
                        command[1]=count;
                        command[3]=change;
                        return 0;
                    }       
                }
            }
            if(numx==1){
                if(buf[1]=='x'){//it is the address that is hex                    
                    if(command[4]==3){//the type spec was used
                        if(sscanf(buf,"0x%x:%c=%u",&address,&type,&change)==3){
                            command[0]=address;
                            command[2]=type;
                            command[3]=change;
                            return 0;
                        }
                        else if(sscanf(buf,"0x%x,%u:%c=%u",&address,&count,&type,&change)==4){
                            command[0]=address;
                            command[1]=count;
                            command[2]=type;
                            command[3]=change;
                            return 0;
                        }
                    } 
                    else{//the type spec was not used
                        if(sscanf(buf,"0x%x=%u",&address,&change)==2){
                            command[0]=address;
                            command[3]=change;
                            return 0;
                        }
                        else if(sscanf(buf,"0x%x,%u=%u",&address,&count,&change)){
                            command[0]=address;
                            command[1]=count;
                            command[3]=change;
                            return 0;
                        }       
                    }
                }
                else{//it is the change val that is hex   
                    if(command[4]==3){//the type spec was used
                        if(sscanf(buf,"%u:%c=0x%x",&address,&type,&change)==3){
                            command[0]=address;
                            command[2]=type;
                            command[3]=change;
                            return 0;
                        }
                        else if(sscanf(buf,"%u,%u:%c=0x%x",&address,&count,&type,&change)==4){
                            command[0]=address;
                            command[1]=count;
                            command[2]=type;
                            command[3]=change;
                            return 0;
                        }
                    } 
                    else{//the type spec was not used
                        if(sscanf(buf,"%u=0x%x",&address,&change)==2){
                            command[0]=address;
                            command[3]=change;
                            return 0;
                        }
                        else if(sscanf(buf,"%u,%u=0x%x",&address,&count,&change)){
                            command[0]=address;
                            command[1]=count;
                            command[3]=change;
                            return 0;
                        }       
                    }
                }
            }
            else{//none of the input is hex
                if(command[4]==3){//the type spec was used
                    if(sscanf(buf,"%u:%c=%u",&address,&type,&change)==3){
                        command[0]=address;
                        command[2]=type;
                        command[3]=change;
                        return 0;
                    }
                    else if(sscanf(buf,"%u,%u:%c=%u",&address,&count,&type,&change)==4){
                        command[0]=address;
                        command[1]=count;
                        command[2]=type;
                        command[3]=change;
                        return 0;
                    }
                }
                else{//the type spec was not used
                    if(sscanf(buf,"%u=%u",&address,&change)==2){
                        command[0]=address;
                        command[3]=change;
                        return 0;
                    }
                    else if(sscanf(buf,"%u,%u=%u",&address,&count,&change)){
                        command[0]=address;
                        command[1]=count;
                        command[3]=change;
                        return 0;
                    }       
                }
            }
        }
    }
    
    return 1;
}

///get start address for load modules
///param: MODULE, section to check
///return: unsigned int starting addr
unsigned int get_start(module_t* MODULE, char* section){
    unsigned int t_starting = 0x00400000;
    unsigned int r_starting = 0x10000000;
    unsigned int d_starting = 0x10000000;
    unsigned int s_starting = 0x10000000;
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    if(MODULE->RDATA){//if rdata is selected and exists
        if(MODULE->DATA){
            int rdata_s = get_size("rdata",MODULE);
            while(rdata_s%8!=0){
                rdata_s++; //get mult of 8 addr
            }
            d_starting+=rdata_s;
        }
        if(MODULE->SDATA){
            s_starting = d_starting;
            int data_s = get_size("data",MODULE);
            while(data_s%8!=0){
                data_s++; //find next mult of 8 addr
            }
            s_starting+=data_s;
        }
    }
    else if(MODULE->DATA){//there is no RDATA but is data
        if(MODULE->SDATA){
            int data_s = get_size("data",MODULE);
            while(data_s%8!=0){
                data_s++;
            }
            s_starting+=data_s;
        }
    }
    //return currect val
    if(!strcmp(section,sections[0])){
        return t_starting;
    }
    else if(!strcmp(section,sections[1])){
        return r_starting;
    }
    else if(!strcmp(section,sections[2])){
        return d_starting;
    }
    else if(!strcmp(section,sections[3])){
        return s_starting;
    }
    else{
        return 0x0;
    }
}

///check command for errors
///param: commands to check, section and module
///return 0 for good 1 for error
int check_for_errors(unsigned int commands[5],char* section, module_t* MODULE){
    //check the type
    unsigned int address = commands[0];
    unsigned int count = commands[1];
    char type = commands[2];
    unsigned int change = commands[3];
    unsigned int flag = commands[4];

    int countsize = 0;

    if(!strcmp("symtab",section)||!strcmp("reltab",section)||!strcmp("reftab",section)){//if a table section
        if(flag==2||flag==3){
            fprintf(stderr,"error: ':%c' not valid in table sections\n",type);
            return 1;
        }
        if(flag==1||flag==3){
            fprintf(stderr,"error: '=%u' is not valid in table sections\n",change);
            return 1;
        }
        countsize = count;
    }
    else{
    //check type
         switch(type){
            case 'b':
                countsize = count;//byte
                break;
            case 'h':
                countsize = count*2;//half
                break;
            case 'w':
                countsize = count*4;//word
                break;
            default:
                fprintf(stderr,"error: '%c' is not a valid type\n",type);
                return 1;
        }
    }

    int offset = 0;
    if(MODULE->HEADER->entry!=0x0){//if its a load module acount for offset
        if(!strcmp(section,"text")){
            offset = get_start(MODULE,"text");
        }
        else if(!strcmp(section,"rdata")){
            offset = get_start(MODULE,"rdata");
        }
        else if(!strcmp(section,"data")){
            offset = get_start(MODULE,"data");
        }
        else if(!strcmp(section,"sdata")){
            offset = get_start(MODULE,"sdata");
        }
    }

    //get size of section;
    unsigned int sect_size = get_size(section,MODULE);
    unsigned int startaddr = 0;
    if(startaddr>(address-offset)||(address-offset)>sect_size){
        if(address==0x0){
            fprintf(stderr,"error: '0' is not a valid address\n");
        }
        else{
            fprintf(stderr,"error: '%u' is not a valid address\n",address);
        }
        return 1;
    }
    if(((address-offset)+countsize)>sect_size){//if the added count and adress will surpass size of section 
        fprintf(stderr,"error: '%d' is not a valid count\n",count);
        return 1;
    }

    return 0;
}

///get the string based on an index

///print out the ref tab
///param: address, count, reference table array, MODULE for string
///return void;
void print_ref_tab(unsigned int address,int count,refent_t* reftab,module_t* MODULE){
    for(int entry=0;entry<count;entry++){
        if(reftab[address].addr == 0x0){
            printf("   0x00000000 type %#06x symbol %s\n",reftab[address].section,&MODULE->STRINGS[ntohl(reftab[address].sym)]);
        }
        else{
            printf("   %#010x type %#06x symbol %s\n",ntohl(reftab[address].addr),reftab[address].section,&MODULE->STRINGS[ntohl(reftab[address].sym)]);
        }
        address++;
    }
}

///print out the rel tab
///param: address, count, rel table array
///return void;i
void print_rel_tab(unsigned int address,int count,relent_t* reltab){
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    for(int entry=0;entry<count;entry++){
        if(reltab[address].addr == 0x0){
            printf("   0x00000000 (%s) type %#06x\n",sections[reltab[address].section-1],reltab[address].type);
        }
        else{
            printf("   %#010x (%s) type %#06x\n",ntohl(reltab[address].addr),sections[reltab[address].section-1],reltab[address].type);
        }
        address++;
    }
}
///print out the sym tab
///param: address, count, sym table array
///return void;
void print_sym_tab(unsigned int address,int count,syment_t* symtab,module_t* MODULE){
    for(int entry=0;entry<count;entry++){
        if(symtab[address].value == 0x0){
            printf("   value 0x00000000 flags %#010x symbol %s\n",ntohl(symtab[address].flags),&MODULE->STRINGS[ntohl(symtab[address].sym)]);
        }
        else{
            printf("   value %#010x flags %#010x symbol %s\n",ntohl(symtab[address].value),ntohl(symtab[address].flags),&MODULE->STRINGS[ntohl(symtab[address].sym)]);
        }
        address++;
    }
}

///print out respective module data
///param: address, count, MODULE
///return void;
void print_module_data(unsigned int address, int count, char type, module_t* MODULE,char* section){
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    for(int s=0;s<10;s++){
        if(!strcmp(section,sections[s])){//if the section was found
            uint8_t* to_print = NULL;
            refent_t* to_print_ref = NULL;
            relent_t* to_print_rel = NULL;
            syment_t* to_print_sym = NULL;

            switch(s){
                case 0://text section
                    to_print = MODULE->TEXT;
                    break;
                case 1://rdata
                    to_print = MODULE->RDATA;
                    break;
                case 2://data
                    to_print = MODULE->DATA;
                    break;
                case 3://sdata
                    to_print = MODULE->SDATA;
                    break;
                case 6://skip bss sbbs to reltab
                    to_print_rel = MODULE->RELTAB;
                    break;
                case 7://reftab
                    to_print_ref = MODULE->REFTAB;
                    break;
                case 8://symtab
                    to_print_sym = MODULE->SYMTAB;
                    break;
                case 9://strings
                    to_print = MODULE->STRINGS;
                    break;
            }

            int offset = 0;
            if(MODULE->HEADER->entry!=0x0){
                if(!strcmp(section,"text")){
                    offset = get_start(MODULE,"text");
                }
                else if(!strcmp(section,"rdata")){
                    offset = get_start(MODULE,"rdata");
                }
                else if(!strcmp(section,"data")){
                    offset = get_start(MODULE,"data");
                }
                else if(!strcmp(section,"sdata")){
                    offset = get_start(MODULE,"sdata");
                }
            }

            if(to_print_ref){
                print_ref_tab(address,count,to_print_ref,MODULE);
            }
            else if(to_print_rel){
                print_rel_tab(address,count,to_print_rel);
            }
            else if(to_print_sym){
                print_sym_tab(address,count,to_print_sym,MODULE);
            }
            else{//if not an entry table
                for(int data=0;data<count;data++){
                    if(type=='b'){
                        if(offset==0x0&&(address-offset)==0x0){
                            if(to_print[address-offset]==0x0){
                                printf("   0x00000000 = 0x00\n");
                            }
                            else{
                                printf("   0x00000000 = %#04x\n",to_print[address-offset]);
                            }
                        }
                        else{
                            if(to_print[address-offset]==0x0){
                                printf("   %#010x = 0x00\n",address);
                            }
                            else{
                                printf("   %#010x = %#04x\n",address,to_print[address-offset]);
                            }
                        }
                        address++;
                    }
                    else if(type=='h'){
                        if(offset==0x0&&(address-offset)==0x0){
                            if(to_print[address-offset]==0x0){
                                printf("   0x00000000 = 0x00%02x\n",to_print[(address-offset)+1]);
                            }
                            else{
                                printf("   0x00000000 = %#04x%02x\n",to_print[address-offset],to_print[(address-offset)+1]);
                            }
                        }
                        else{
                            if(to_print[address-offset]==0x0){
                                printf("   %#010x = 0x00%02x\n",address,to_print[(address-offset)+1]);
                            }
                            else{
                                printf("   %#010x = %#04x%02x\n",address,to_print[address-offset],to_print[(address-offset)+1]);
                            }
                        }
                        address+=2;
                    }
                    else{
                        if(offset==0x0&&(address-offset)==0x0){
                            if(to_print[address-offset]==0x0){
                                printf("   0x00000000 = 0x00%02x%02x%02x\n",to_print[(address-offset)+1],to_print[(address-offset)+2],to_print[(address-offset)+3]);
                            }
                            else{
                                 printf("   0x00000000 = %#04x%02x%02x%02x\n",to_print[address-offset],to_print[(address-offset)+1],to_print[(address-offset)+2],to_print[(address-offset)+3]);
                            }
                        }
                        else{
                            if(to_print[address-offset]==0x0){
                                printf("   %#010x = 0x00%02x%02x%02x\n",address,to_print[(address-offset)+1],to_print[(address-offset)+2],to_print[(address-offset)+3]);
                            }
                            else{
                                printf("   %#010x = %#04x%02x%02x%02x\n",address,to_print[address-offset],to_print[(address-offset)+1],to_print[(address-offset)+2],to_print[(address-offset)+3]);
                            }
                        } 
                        address+=4;
                    }
                }
            }
        }
    }
}


///funciton to edit the modules memory in place
///param: address,count,type,change value,MODULE,seciton
void edit_module_data(unsigned int address,int count,char type,unsigned int change,module_t* MODULE,char* section){
    char* sections[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
    for(int sec=0;sec<10;sec++){
        if(!strcmp(section,sections[sec])){
            uint8_t byte_toadd = 0;
            uint8_t half_toadd[2] = {0};
            uint8_t word_toadd[4] = {0};
            
            uint8_t* to_write = NULL;

            switch(sec){
                case 0:
                    to_write = MODULE->TEXT;
                    break;
                case 1:
                    to_write = MODULE->RDATA;
                    break;
                case 2:
                    to_write = MODULE->DATA;
                    break;
                case 3:
                    to_write = MODULE->SDATA;
                    break;
                case 9:
                    to_write = MODULE->STRINGS;
                    break;
            }

            int offset = 0;
            if(MODULE->HEADER->entry!=0x0){
                if(!strcmp(section,"text")){
                    offset = get_start(MODULE,"text");
                }
                else if(!strcmp(section,"rdata")){
                    offset = get_start(MODULE,"rdata");
                }
                else if(!strcmp(section,"data")){
                    offset = get_start(MODULE,"data");
                }
                else if(!strcmp(section,"sdata")){
                    offset = get_start(MODULE,"sdata");
                }
            }

            switch(type){
                case 'b':
                    if(change>=0xff){//if bigger than a byte
                        byte_toadd = 0xff;
                    }
                    else{
                        byte_toadd = change;
                    }
                    break;
                case 'h':
                    if(change>=0xffff){//greater than half word
                        half_toadd[0] = 0xff;
                        half_toadd[1] = 0xff;
                    }
                    else{
                        unsigned mask = createMask(0,7);//create an 8 bit mask
                        half_toadd[1] = mask & change;//flip because bigendian
                        change = change>>8;//shift 8 bits
                        half_toadd[0] = mask & change;
                    }
                    break;
                case 'w':
                    if(change>=0xffffffff){//greater than a word
                        word_toadd[0] = 0xff;
                        word_toadd[1] = 0xff;
                        word_toadd[2] = 0xff;
                        word_toadd[3] = 0xff;
                    }
                    else{
                        unsigned mask = createMask(0,7);
                        word_toadd[3] = mask & change;
                        change = change>>8;
                        word_toadd[2] = mask & change;
                        change = change>>8;
                        word_toadd[1] = mask & change;
                        change = change>>8;
                        word_toadd[0] = mask & change;
                    }
                    break;
            }
            if(type=='b'){//write the byte
                for(int byte=0;byte<count;byte++){
                    to_write[address-offset] = byte_toadd;
                    if(address==0x0){
                        if(to_write[address-offset]==0x0){
                            printf("   0x00000000 is now 0x00\n");
                        }
                        else{
                            printf("   0x00000000 is now %#04x\n",to_write[address-offset]);
                        }
                    }
                    else{
                        if(to_write[address-offset]==0x0){
                            printf("   %#010x is now 0x00\n",address);
                        }
                        else{
                            printf("   %#010x is now %#04x\n",address,to_write[address-offset]);
                        }
                    }
                    address++;
                }
            }
            else if(type=='h'){//write a half word
                for(int half=0;half<count;half++){
                    for(int byte=0;byte<2;byte++){//2 bytes in a half
                        to_write[(address-offset)+byte] = half_toadd[byte];
                    }
                    if(address==0x0){
                        if(half_toadd[0]==0x0){
                            printf("   0x00000000 is now 0x00%02x\n",half_toadd[1]);
                        }
                        else{
                            printf("   0x00000000 is now %#04x%02x\n",half_toadd[0],half_toadd[1]);
                        }
                    }
                    else{
                        if(half_toadd[0]==0x0){
                            printf("   %#010x is now 0x00%02x\n",address,half_toadd[1]);
                        }
                        else{
                            printf("   %#010x is now %#04x%02x\n",address,half_toadd[0],half_toadd[1]);
                        }
                    }
                    address+=2;
                }
            }
            else if(type=='w'){//write a word
                for(int word=0;word<count;word++){
                    for(int byte=0;byte<4;byte++){
                        to_write[(address-offset)+byte] = word_toadd[byte];
                    }
                    if(address==0x0){
                        if(word_toadd[0]==0x0){
                            printf("   0x00000000 is now 0x00%02x%02x%02x\n",word_toadd[1],word_toadd[2],word_toadd[3]);
                        }
                        else{
                            printf("   0x00000000 is now %#04x%02x%02x%02x\n",word_toadd[0],word_toadd[1],word_toadd[2],word_toadd[3]);
                        }
                    }
                    else{
                        if(word_toadd[0]==0x0){
                            printf("   %#010x is now 0x00%02x%02x%02x\n",address,word_toadd[1],word_toadd[2],word_toadd[3]);
                        }
                        else{
                            printf("   %#010x is now %#04x%02x%02x%02x\n",address,word_toadd[0],word_toadd[1],word_toadd[2],word_toadd[3]);
                        }
                    }
                    address+=4;
                }
            }
        }
    }
}

///fuction to edit the module based on the command
///param: MODULE module to edit, command command to process, current section
///return 1 if written 0 if examined
int edit_module(module_t* MODULE, unsigned int commands[5],char* section){
    //printf("[%#x][%d][%c][%#x] change flag: [%d]\n",commands[0],commands[1],commands[2],commands[3],commands[4]); 
    if(!check_for_errors(commands,section,MODULE)){
        //the error test passed
        if(commands[4]==1||commands[4]==3){//if values will be changed
            edit_module_data(commands[0],commands[1],commands[2],commands[3],MODULE,section); 
            return 1;
        }
        else{//if its just a print command
            print_module_data(commands[0],commands[1],commands[2],MODULE,section);
            return 0;
        }
    }
    return 0;
}

///handle input 
int run(module_t* MODULE,char* file){
    char current_sec[10] = "text";
    int seq = 1;
    char buf[128]={0};
    char sect[32]={0};
    //history data
    history_cmd_t* history[10] = {0};
    int hist_s = 0;
    //flags
    int da_flag = 0;
    int readin = 1;
    int changed = 0;
    //
    int sequence = 0;
    unsigned int x_command[5]={0};//array to hold the examine command
    while(1){//get input
        da_flag = 0;
        if(readin){
            printf("%s[%d] > ",current_sec,seq);
            fgets(buf,128,stdin);
        }
        readin=1;
        if(strcmp(buf,"\n") && (!strcmp(strtok(buf,"\n"),"quit"))){
            //quit
            char ans[256] = {0};
            if(changed){
                printf("Discard modifications (yes or no)?");
                fgets(ans,256,stdin);
                char* answer = strtok(ans,"\n");
                if(!strcmp(answer,"yes")){
                    destroy_history(history,&hist_s);
                    destroy_module(MODULE);
                    exit(0);
                }
            }
            else{
                destroy_history(history,&hist_s);
                return 0;
            }
        }
        else if(strcmp(buf,"\n") && (!strcmp(strtok(buf,"\n"),"size"))){
            //size
            char* unit = "bytes";
            if(!strcmp(current_sec,"reltab")||!strcmp(current_sec,"reftab")||!strcmp(current_sec,"symtab")){
                unit = "entries";
            }
            int size = get_size(current_sec,MODULE);
            printf("Section %s is %d %s long\n",current_sec,size,unit);
        }
        else if(strcmp(buf,"\n") && (!strcmp(strtok(buf,"\n"),"write"))){
            //write
            if(changed){ 
                write(MODULE,file);
                changed = 0;
            }
            else{
                printf("There have been no changes: nothing to write\n");
            }
        }
        else if(strcmp(buf,"\n") && (!strcmp(strtok(buf,"\n"),"history"))){
            //history
            add_to_history(buf,seq,history,&hist_s);
            da_flag = 1;
            for(int entry=0;entry<hist_s;entry++){
                printf("%d  %s\n",history[entry]->seqnum,history[entry]->command);
            }
        }
        else{//check if its the section command or the examination/modify
            if(sscanf(buf,"section %s",sect)==1){
                //section
                if(!strcmp(sect,"sbss")||!strcmp(sect,"bss")){
                    fprintf(stderr,"error: cannot edit %s section\n",sect);
                }
                else{
                    int in = 0;
                    char* valid[] = {"text","rdata","data","sdata","sbss","bss","reltab","reftab","symtab","strings"};
                    for(int i=0;i<10;i++){
                        if(!strcmp(sect,valid[i])){
                            if(i!=4&&i!=5){//its not bss or sbss  
                                int size = MODULE->HEADER->data[i];
                                if(size){
                                    in = 1;
                                }
                                else{//the section doesnt exist
                                    fprintf(stderr,"error: the section '%s' is not present in this module\n",sect);
                                    in = 2;
                                }
                            }
                        }
                    }
                    if(in==1){
                        printf("Now editing section %s\n",sect);
                        strncpy(current_sec,sect,strlen(sect)+1);
                    }
                    else{
                        if(in==0){
                            fprintf(stderr,"error: '%s' is not a valid section name\n",sect);
                        }
                    }
                }
            }
            else if(sscanf(buf,"!%d",&sequence)==1){
                //sequennce retrieve
                if(history[0]){
                    int lowest = history[0]->seqnum;
                    int highest = history[hist_s-1]->seqnum;
                    if(sequence<lowest){
                        fprintf(stderr,"error: command %d is no longer in the command history\n",sequence);
                    }
                    else if(sequence>highest){
                        fprintf(stderr,"error: command %d has not yet been entered\n",sequence);
                    }
                    else{
                        for(int entry=0;entry<hist_s;entry++){
                            if(sequence==history[entry]->seqnum){
                                printf("%s[%d] > %s\n",current_sec,seq,history[entry]->command);
                                strncpy(buf,history[entry]->command,strlen(history[entry]->command)+1);                                
                                readin=0;
                            }
                        }
                    }
                }
                else{
                    fprintf(stderr,"error: command %d has not yet been entered\n",sequence);
                }
            }
            else{
                if(!proccess_x_command(x_command,buf)){
                    if(edit_module(MODULE,x_command,current_sec)){
                        changed=1;
                    }
                }
            }
        }
        if(!da_flag&&readin==1){
            add_to_history(buf,seq,history,&hist_s);
        }
        if(readin==1){
            seq++;
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    if(argc!=2){
        fprintf(stderr,"usage: lmedit file\n");
        return 1;
    }
    else{
        //data
        module_t* MODULE = calloc(1,sizeof(module_t));
        exec_t* header = calloc(1,sizeof(exec_t));
        
        //
        FILE* mfp = open_module(argv[1]);
        if(!mfp){//if the file couldnt be opened or wasnt a R2K
            exit(EXIT_FAILURE);
        }
        else{
            header->magic = HDR_MAGIC;
        }
        //begin load module into memory
        //first populate header
        fread(&(header->version),sizeof(uint16_t),1,mfp);//get next 16 bits for the version;
        fseek(mfp,4,SEEK_CUR);//skip ahead 4 bytes
        fread(&(header->entry),sizeof(uint32_t),1,mfp);//get the next 32 bits for the entry
        //get the section sizes
        for(int section=0;section<N_EH;section++){
            if(!fread(&((header->data)[section]),sizeof(uint32_t),1,mfp)){//read in the section size
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }
        //store the header in the MODULE
        MODULE->HEADER = header;
        //gather the TEXT, RDATA, DATA, SDATA, SBSS, and BSS and store in MODULE
        //TEXT
        if((header->data)[EH_IX_TEXT]!=0){
            MODULE->TEXT = calloc(1,ntohl((header->data)[EH_IX_TEXT]));
            if(!fread((MODULE->TEXT),ntohl((header->data)[EH_IX_TEXT]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }
        else{
            MODULE->TEXT = NULL;
        }
        //RDATA
        if((header->data)[EH_IX_RDATA]!=0){
            MODULE->RDATA = calloc(1,ntohl((header->data)[EH_IX_RDATA]));
            if(!fread((MODULE->RDATA),ntohl((header->data)[EH_IX_RDATA]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }
        else{
            MODULE->RDATA = NULL;
        }
        //DATA
        if((header->data)[EH_IX_DATA]!=0){
            MODULE->DATA = calloc(1,ntohl((header->data)[EH_IX_DATA]));
            if(!fread((MODULE->DATA),ntohl((header->data)[EH_IX_DATA]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }   
        else{
            MODULE->DATA = NULL;
        }
        //SDATA
        if((header->data)[EH_IX_SDATA]!=0){
            MODULE->SDATA = calloc(1,ntohl((header->data)[EH_IX_SDATA]));
            if(!fread((MODULE->SDATA),ntohl((header->data)[EH_IX_SDATA]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }   
        else{
            MODULE->SDATA = NULL;
        }
        //SBSS
        if((header->data)[EH_IX_SBSS]!=0){
            MODULE->SBSS = calloc(1,ntohl((header->data)[EH_IX_SBSS]));
            if(!fread((MODULE->SBSS),ntohl((header->data)[EH_IX_SBSS]),1,mfp)){
               perror(argv[1]);
               destroy_module(MODULE);
               exit(EXIT_FAILURE);
            }
        }   
        else{
            MODULE->SBSS = NULL;
        }
        //BSS
        if((header->data)[EH_IX_BSS]!=0){
            MODULE->BSS = calloc(1,ntohl((header->data)[EH_IX_BSS]));
            if(!fread((MODULE->BSS),ntohl((header->data)[EH_IX_BSS]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }   
        else{
            MODULE->BSS = NULL;
        }
        //gather tables
        //allocate memory
        if((header->data)[EH_IX_REL]!=0){
            MODULE->RELTAB = calloc(ntohl((header->data)[EH_IX_REL]),sizeof(relent_t));
            for(unsigned int entry=0;entry<ntohl((header->data)[EH_IX_REL]);entry++){
                int a = fread(&(((MODULE->RELTAB)[entry]).addr),sizeof(uint32_t),1,mfp);
                int s = fread(&(((MODULE->RELTAB)[entry]).section),sizeof(uint8_t),1,mfp);
                int t = fread(&(((MODULE->RELTAB)[entry]).type),sizeof(uint8_t),1,mfp);
                if(!(a&&s&&t)){
                    perror(argv[1]);
                    destroy_module(MODULE);
                    exit(EXIT_FAILURE);
                }
                fseek(mfp,2,SEEK_CUR);
            }
        }
        else{
            MODULE->RELTAB = NULL;
        }
        if((header->data)[EH_IX_REF]!=0){
            MODULE->REFTAB = calloc(ntohl((header->data)[EH_IX_REF]),sizeof(refent_t));
            for(unsigned int entry=0;entry<ntohl((header->data)[EH_IX_REF]);entry++){
                
                int a = fread(&(((MODULE->REFTAB)[entry]).addr),sizeof(uint32_t),1,mfp);
                int s = fread(&(((MODULE->REFTAB)[entry]).sym),sizeof(uint32_t),1,mfp);
                fseek(mfp,1,SEEK_CUR);
                int e = fread(&(((MODULE->REFTAB)[entry]).section),sizeof(uint8_t),1,mfp);
                if(!(a&&s&&e)){
                    perror(argv[1]);
                    destroy_module(MODULE);
                    exit(EXIT_FAILURE);
                }
                fseek(mfp,2,SEEK_CUR);
            }
        }
        else{
            MODULE->REFTAB = NULL;
        }
        if((header->data)[EH_IX_SYM]!=0){
            MODULE->SYMTAB = calloc(ntohl((header->data)[EH_IX_SYM]),sizeof(syment_t));
            for(unsigned int entry=0;entry<ntohl((header->data)[EH_IX_SYM]);entry++){
                int f = fread(&(((MODULE->SYMTAB)[entry]).flags),sizeof(uint32_t),1,mfp);
                int v = fread(&(((MODULE->SYMTAB)[entry]).value),sizeof(uint32_t),1,mfp);
                int s = fread(&(((MODULE->SYMTAB)[entry]).sym),sizeof(uint32_t),1,mfp);
                if(!(f&&v&&s)){
                    perror(argv[1]);
                    destroy_module(MODULE);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else{
            MODULE->SYMTAB = NULL;
        }
        //read in the text
        if((header->data)[EH_IX_STR]!=0){
            MODULE->STRINGS = calloc(1,ntohl((header->data)[EH_IX_STR]));
            if(!fread((MODULE->STRINGS),ntohl((header->data)[EH_IX_STR]),1,mfp)){
                perror(argv[1]);
                destroy_module(MODULE);
                exit(EXIT_FAILURE);
            }
        }
        else{
            MODULE->STRINGS = NULL;
        }
        //finished loading in module
        fflush(mfp);
        fclose(mfp);
        
        //print the summary
        print_summary(header,argv[1]);
        //begin command loop
        run(MODULE,argv[1]);

        //cleanup
        destroy_module(MODULE);
    }
}
