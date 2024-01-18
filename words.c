#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
	int occurance;
	char* word;
}wordFreq;

int compare(const void* a,const void* b){
	const wordFreq* w1 = (wordFreq*) a;
	const wordFreq* w2 = (wordFreq*) b;
	if ((*w1).occurance>(*w2).occurance){return -1;}
	if ((*w1).occurance<(*w2).occurance){return 1;}
	return strcmp(w1->word, w2->word);
}

static char* increaseArrSize(char* arr, int size){
	return (char*)realloc(arr,size+32);
}

static void printWordCounter(wordFreq* wc){
        qsort(&(wc[2]),wc[1].occurance,sizeof(wordFreq),compare);
	int i=2;
        while(i<wc[1].occurance+2){
                write(1,wc[i].word,strlen(wc[i].word));
		write(1," ",1);
		char buffer[20];
		int numDigits = sprintf(buffer, "%d", wc[i].occurance);
		write(1, buffer, numDigits);
		write(1,"\n",1);
		//printf("%s = %d\n",wc[i].word,wc[i].occurance);
                i++;
        }
}

static int isLetter(char a){
	//check ASCII value of passed character, if char is a letter (lowercase or uppercase), return 1.
	//If not a character, return 0
	if((  (int)a >= 65 && (int)a <= 90) || ((int)a >= 97 && (int)a <= 122)  ){
		return 1;
	}
	return 0;
}
static wordFreq* endWord (int currWordLength, char* currentWord, wordFreq* wordCounter){
        //add null terminator to end of array
        currentWord[currWordLength] = '\0';
        //check if string is in array
        int i=2;
        int wordFound=0;
        while(i<wordCounter[1].occurance+2){
                if(!strcmp(wordCounter[i].word,currentWord)){
                        wordFound=1;
                        break;
                }
	        i++;
	}
//	printf("wordFound = %d\n",wordFound);
	if(wordFound){
  //      	printf("word found\n");
        	wordCounter[i].occurance++;
	}
	else{
	        //resize wordCounter array if it is full
	        if(wordCounter[0].occurance-2==wordCounter[1].occurance){
	                wordCounter = realloc(wordCounter,wordCounter[0].occurance+sizeof(wordFreq)*32);
	                wordCounter[0].occurance+=32;
	        }
	        wordCounter[2+wordCounter[1].occurance].word = (char*)malloc(currWordLength+1);
	        //Add string to the array
	        strcpy(wordCounter[2+wordCounter[1].occurance].word,currentWord);
	        //update wordCounter array size
            wordCounter[2+wordCounter[1].occurance].occurance = 1;
			wordCounter[1].occurance++;
      }
	return wordCounter;
}

static wordFreq* readFile (char* textFile, wordFreq* wordCounter){
	int fd = open(textFile,O_RDONLY);
        if(fd==-1){
                printf("Error: Could not open file to read with filename :%s.",textFile);

		return wordCounter;
        }
    char prev;
	char c='!'; //initialize to non character so if first letter is a - or ', prev will be set to non-character
    int status = 1;
	char* currentWord = malloc(sizeof(char)*32); //started with 32 so that don't have to realloc
	int currWordLength = 0;
	int arrLength = 32;
	//boolean that represents if the code found a potentially valid "'" or "-"
	int foundDash=0;

	while(1){

		prev = c;
                status =  read(fd,&c,1);
                if(status==0||status==-1){
			break;
		}
		//increase array size before adding anything to array if array is full
		if(currWordLength==arrLength){
			currentWord = increaseArrSize(currentWord, arrLength);
			arrLength+=32;
		}
		if(foundDash){
			 if(isLetter(c)){
				currentWord[currWordLength] = '-';
				currWordLength++;

				//resize if array full
				if(currWordLength==arrLength){
						currentWord = increaseArrSize(currentWord, arrLength);
						arrLength+=32;
				}
			}
			else if(currWordLength>0){
				wordCounter = endWord(currWordLength, currentWord, wordCounter);
				currWordLength=0;
			}
			foundDash=0;
		}
		if(isLetter(c) || c=='\''){
			currentWord[currWordLength] = c;
			currWordLength++;
		}

		//if c is a potentially valid "-" or a "'", then set found to true
		else if(c=='-' && isLetter(prev)) {
			foundDash=1;
		}
		else if(currWordLength>0){
                        wordCounter = endWord(currWordLength, currentWord, wordCounter);
			currWordLength=0;
		}
	}
	if(currWordLength>0){
		wordCounter = endWord(currWordLength, currentWord, wordCounter);
		currWordLength=0;
	}
	close(fd);
	return wordCounter;
}

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        // There was an error getting the file status
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}


static wordFreq* recursiveSearch(wordFreq* wc, char* dirName){
	DIR* dir = opendir(dirName);
    struct dirent* ptr;
	//initialize the full path of the directory
	char* filePath = malloc(sizeof(char)*1);
	//stop if we run out of files
    while((ptr = readdir(dir))!= NULL){
		//malloc a large enough char* to store the file path we are currently looking at
		filePath = (char*)realloc(filePath, 4+sizeof(char)*(strlen(dirName)+strlen(ptr->d_name)));
		//excluding self and parent to avoid infinite loop
		if(strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0){
			//concatenate file directory names together to create a full path
			sprintf(filePath, "%s/%s", dirName, ptr->d_name);
		}
		if(is_directory(filePath)){
			//if we reach a directory
			recursiveSearch(wc, filePath);
		} else if(strcmp(filePath + strlen(filePath)-4, ".txt") == 0){
			//if we reach a textfile read the file and note down the words
			wc = readFile(filePath, wc);
		}


    }
    closedir(dir);
	//no memory leaks!!
	free(filePath);
	return wc;
}

int main(int argc, char* argv[]){
	//first element of the array is the array size.
	//second element of the array is number of used indexes.
	wordFreq* wc = malloc(sizeof(wordFreq)*8);
	wc[0].occurance=8;
	wc[1].occurance=0;

	char* path = malloc(sizeof(char));
	for(int i = 1; i < argc; i++){
		path = realloc(path,sizeof(char)*(strlen(argv[i])+2));
		sprintf(path,"./%s", argv[i]);
		if(is_directory(path)){
			wc = recursiveSearch(wc, path);
		} else {
			wc = readFile(argv[i], wc);
		}
	}
	//wc=readFile(argv[2],wc);

	printWordCounter(wc);



	return 0;
}
