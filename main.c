#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define READ_FILE_NAME "code.txt"
#define WRITE_FILE_NAME "output.txt"
#define MAX_LINE_LENGTH 100

// While we are dealing with for loop, we should store step and write end of the level 
char step[MAX_LINE_LENGTH] = {0};

// After level finished, we should use level name to write END <Level Name>
char levelName[8][MAX_LINE_LENGTH];

// Type of Level in terms of enum OPERATION
int levelType[8];

// In order to use file pointer in all functions without sending as parameter 
// We can use global variable
FILE *writeFile, *readFile;

// Reads line and returns some cases such as:
// 0 -> continue as usual
// 1 -> curlybracket opened, new level alert
// 2 -> curlybracket closed, level finished
// 3 -> should  curlybracket open, but not yet
int readLine(char*, int, bool);

// recursive function that reads level informations
// all cases checked in this function such as:
// *Level Ends
// *One Lined Levels and so on.
void readLevel(int);

// With coming operation, check cases and write to file according to operation and line
void writeToFile(int, const char*, int);

// Removes indentation spaces
char* removeSpaces(char*);

// Sets Level name chars to '\0'
void resetLevelName(int);

// Sets step chars to '\0'
void resetStep();

// For readablity, operation types named with enum
enum OPERATION {
    IF,
    ELSEIF,
    ELSE,
    FUNCTION_DEF,
    FUNCTION_CALL,
    FOR,
    WHILE,
    RETURN,
    ASSIGNMENT,
    EMPTY
};

// Finds operation to perform in line
int findOperation(char* line, int level){
    line = removeSpaces(line);
    if (strncmp(line, "if", 2) == 0) {
        return IF;
    }
    if (strncmp(line, "else if", 7) == 0) {
        return ELSEIF;
    }
    if (strncmp(line, "else", 4) == 0) {
        return ELSE;
    }
    if (strncmp(line, "for", 3) == 0) {
        return FOR;
    }
    if (strncmp(line, "while", 5) == 0) {
        return WHILE;
    }
    if (strncmp(line, "return", 6) == 0) {
        return RETURN;
    }
    if (strstr(line, "=")) {
        return ASSIGNMENT;
    }
    if(strstr(line,"(") && strstr(line, ";")){
        return FUNCTION_CALL;
    }
    if(level == 0 && line[0] != '#')
        return FUNCTION_DEF;
    
    return EMPTY;
}



int main(){
    for(int i = 0; i < 8; i++)
        levelType[i] = -1;
    readFile = fopen(READ_FILE_NAME, "r");
    if(!readFile)
        printf("Error opening the read file.\n");

    writeFile = fopen(WRITE_FILE_NAME, "w+");
    if(!writeFile)
        printf("Error opening the write file.\n");

    // Since this is a recursive function this is all we need.
    readLevel(0);
    fclose(readFile);
    fclose(writeFile);
    return 0;
}



// Reads each line. Calls writeToFile for necessary operation.
int readLine(char* line, int level, bool expectingCurlyBrackets){

    bool isOneLine = false;
    line = removeSpaces(line);

    // If the line is empty or starts is an #include line.
    if(strlen(line) == 0 || line[0] == '#')
        return 0;

    // If we are expecting curly brackets
    if(expectingCurlyBrackets){
        if(line[0] == '{')
            return 1;
        else
            isOneLine = true;
    }
    // if a block finished
    if(line[0] == '}')
        return 2;

    // do nothing
    int operation = findOperation(line, level);
    if(operation == EMPTY)
        return 0;
    
    // since we are not calling readLevel for one line codes, we are adding tab ourselves
    if(isOneLine)
        fprintf(writeFile,"\t");
    writeToFile(operation, line, level);

    // in other cases, there can be a starting of a new level. so that, return 1 and say "new level"
    // if there is not => "we are excpecting a opening"
    if(operation != ASSIGNMENT && operation != RETURN && operation != FUNCTION_CALL){
        if(strstr(line, "{")){
            return 1;
        }
        else
            return 3;
    }
    return 0;

}


void readLevel(int level){
    char line[MAX_LINE_LENGTH];
    bool expectingCurlyBrackets = false;
    bool isIfStarted = false;
    bool isIfFinished = false;
    bool isOneLine = false;

    while(fgets(line, MAX_LINE_LENGTH, readFile) != NULL){
        int operation = findOperation(line, level);
        int ret = readLine(line, level, expectingCurlyBrackets);

        // if there is a starting if statement and coming parts are not else if or else
        // then if finished
        if(isIfStarted && !(operation == ELSE || operation == ELSEIF)){
            isIfStarted = false;
            isIfFinished = true;
            
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t"); 
            fprintf(writeFile, "ENDIF\n");
            levelType[level] = -1;
            isIfFinished = false;
        }
        if(operation == IF){
            isIfStarted = true;
        }
        if(operation == IF || operation == FOR || operation == FUNCTION_DEF || operation == WHILE){
            levelType[level] = operation;
        }
        // initially assume false
        expectingCurlyBrackets = false;
        if(ret == 1){ // curlybrackets opened

            //call recursively new level reading function
            readLevel(level + 1);
            // after returning upper level, check END statement
            if(isIfFinished){
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t"); 
                fprintf(writeFile, "ENDIF\n");
                levelType[level] = -1;
                isIfFinished = false;
            }
            else if(levelType[level] == FOR){
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
                fprintf(writeFile, "\t%s\n", step);
                resetStep();
                
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
                fprintf(writeFile, "END WHILE\n");
                levelType[level] = -1;
            }
            else if(levelType[level] == FUNCTION_DEF){
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
                fprintf(writeFile, "END FUNCTION %s\n", levelName[level]);
                levelType[level] = -1;
            }
            else if(levelType[level] == WHILE){
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
                fprintf(writeFile, "END WHILE\n");
                levelType[level] = -1;
            }
            
            resetLevelName(level);
        }

        else if(ret == 2) // closed
            return;
        else if(ret == 3) // expecting signal
            expectingCurlyBrackets = true;
        
    }
}

char* removeSpaces(char *str) {
    
    for(int i =0; i < strlen(str); i++){
        if(str[i] != ' '){
            return str + i;
        }
    }
    return "";
}

void writeToFile(int operation, const char* line, int level){
    
    switch (operation)
    {

    case IF:
        {
            strcpy(levelName[level], "IF");
            int startIndex = 0;
            int endIndex = 0;

            // find indexes of condition
            for(int i = 0; i < strlen(line); i++){
            
                if(line[i] == '(')
                    startIndex = i + 1;
                
                if(line[i] == ')')
                    endIndex = i;

            }
            
            char condition[MAX_LINE_LENGTH] = {0};
            // write into condition
            strncpy(condition, line + startIndex, endIndex - startIndex);
            
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            fprintf(writeFile, "IF %s THEN\n", condition);
            break;
        }
    case ELSEIF:
        
        {
            int startIndex = 0;
            int endIndex = 0;
            for(int i = 0; i < strlen(line); i++){
            // find indexes of condition
            if(line[i] == '(')
                startIndex = i + 1;
            
            else if(line[i] == ')')
                endIndex = i;
            
            }

            char condition[MAX_LINE_LENGTH] = {0};
            //write into condition
            strncpy(condition, line + startIndex, endIndex - startIndex);
            
            
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            fprintf(writeFile, "ELSEIF %s THEN\n", condition);
            break;
        }

    case ELSE:
    
        {
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            fprintf(writeFile, "ELSE\n");
            break;
        }
        
    case FUNCTION_DEF:
        
        {
            char functionName[MAX_LINE_LENGTH] = {0};
            char insideTheParenthesis[MAX_LINE_LENGTH] = {0};
                
            int nameBeginIndex = 0;
            int nameEndIndex = 0;

            int parenthesisBeginIndex = 0;
            int parenthesisEndIndex = 0;
            
            // Find name begin index.
            for(int i = 0; i < strlen(line); i++){
                if(line[i] == ' '){
                    nameBeginIndex = i + 1;
                    break;
                }
            }

            // Find name end index.
            for(int i = nameBeginIndex; i < strlen(line); i++){
                if(line[i] == ' ' || line[i] == '('){
                    nameEndIndex = i;
                    break;
                }
            }

            // Find variables in the function
            for(int i = nameEndIndex; i < strlen(line); i++){
                if(line[i] == '(')
                    parenthesisBeginIndex = i;
                else if(line[i] == ')')
                    parenthesisEndIndex = i;
            }
            
            strncpy(functionName, line + nameBeginIndex, nameEndIndex - nameBeginIndex);
            strcpy(levelName[0], functionName);
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            fprintf(writeFile, "FUNCTION %s ", functionName);
            // We included ')' in order to use it to find variable names in the upcoming for loop.
            strncpy(insideTheParenthesis, line + parenthesisBeginIndex + 1, parenthesisEndIndex - parenthesisBeginIndex - 1);
            if (strlen(insideTheParenthesis) == 1) // Therefore insideTheParenthesis will be empty when the length is 1 since it always contains ')'
                break; // If it's empty then we just break

            for(int i = 0; i < strlen(insideTheParenthesis); i++){
                if(insideTheParenthesis[i] == ',' || insideTheParenthesis[i] == ')'){
                    int j = i;
                    int commaIndex = i;
                    char variableName[MAX_LINE_LENGTH] = {0};

                    // int main(int ahmet, int b) ,-->9
                    // space--> 3
                    // Until you find a space that is not 1 index away from ','
                    while(!(insideTheParenthesis[j] == ' ' && commaIndex - j != 1)){
                        j--;
                    }
                    int variableNameBeginIndex = j + 1;
                    
                    strncpy(variableName, insideTheParenthesis + variableNameBeginIndex, commaIndex - variableNameBeginIndex);
                    fprintf(writeFile ,"%s ", variableName);
                }
            }

            fprintf(writeFile ,"\n");    
            break;
        }

    case FUNCTION_CALL:
        {int nameEndIndex = 0;
        char functionName[MAX_LINE_LENGTH] = {0};
        int parenthesisBeginIndex = 0;
        int parenthesisEndIndex = 0;
        
        // name begin index
        for(int i = 0; i < strlen(line); i++){
            if(line[i] == ' ' || line[i] == '('){
                nameEndIndex = i;
                break;
            }
            functionName[i] = line[i];
        }
        
        // find parenthesis indexes
        for(int i = nameEndIndex; i < strlen(line); i++){
            if(line[i] == '(')
                parenthesisBeginIndex = i;
            else if(line[i] == ')')
                parenthesisEndIndex = i;
        }

        // Indentation
        for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");

        // check if function is printf, if so write PRINT <variable>
        if(strstr(functionName, "printf")){
            fprintf(writeFile,"PRINT ");
            char* beginVariable = strstr(line, ",");
            beginVariable++;
            for(int i = 0; beginVariable[i] != ')'; i++){
                fprintf(writeFile ,"%c", beginVariable[i]);
            }
            fprintf(writeFile, "\n");
            break;
        }

        // check if function is scanf, if so write READ <variable>
        if(strstr(functionName, "scanf")){
            fprintf(writeFile,"READ ");
            char* beginVariable = strstr(line, "&");
            beginVariable++;
            for(int i = 0; beginVariable[i] != ')'; i++){
                fprintf(writeFile ,"%c", beginVariable[i]);
            }
            fprintf(writeFile, "\n");
            break;
        }
        
        fprintf(writeFile, "%s ", functionName);
        // write variables in parenthesis without comma
        for(int i = parenthesisBeginIndex + 1; i < parenthesisEndIndex; i++){
            if(line[i] == ',')
                fprintf(writeFile , " ");
            else
                fprintf(writeFile, "%c", line[i]);
        }
        fprintf(writeFile, "\n");
        break;}

    case FOR:
        {
            strcpy(levelName[level], "WHILE");
            char forInfo[3][20];
            for(int i = 0; i < 3; i++){
                for(int j = 0; j < 20; j++){
                    forInfo[i][j] = '\0';
                }
            }

            // Finding where the parenthesis begin and end.
            int parenthesisBeginIndex, parenthesisEndIndex;
            for(int i = 0; i < strlen(line); i++){
                if(line[i] == '(')
                    parenthesisBeginIndex = i;
                if(line[i] == ')')
                    parenthesisEndIndex = i;
            }

            // writeInfo states which part of the for loop we are in.
            int writeInfo = 0;
            // Index stands for the index of the assignment, condition or the step.
            int writeIndex = 0;

            // Seperating and writing every part of the for loop.
            for(int i = parenthesisBeginIndex; i < parenthesisEndIndex; i++){
                if(line[i] == ';'){ // When a ; is found we skip into the next part.
                    writeInfo++;
                    writeIndex = 0;
                    continue;
                }
                
                forInfo[writeInfo][writeIndex] = line[i];
                writeIndex++;
            }


            // Copying step.
            strcpy(step, forInfo[2]);
            // Indentation
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            // Printing 
            fprintf(writeFile, "%s\n", forInfo[0] + 1);
            // Indentation
            for(int i = 0; i < level; i++)
                fprintf(writeFile, "\t");
            fprintf(writeFile, "WHILE %s\n", forInfo[1]);
            break;
        }

    case WHILE:
        {
            // get condition with finding '('
            char* whileCondition = strstr(line, "(");
            whileCondition++;
            // Indentation
            for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
            fprintf(writeFile, "WHILE ");
            // write it without );
            for(int i = 0; i < strlen(whileCondition) - 2; i++){
                fprintf(writeFile, "%c", whileCondition[i]);
            }
            fprintf(writeFile, "\n");
            strcpy(levelName[level], "WHILE");
            break;
        }
        
    case RETURN:
        {
            // returnValue stars where a space is found.
            char* returnValue = strstr(line, " ");
            // Indentation
            for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");
                    
            // Writing levelName which is the function name and the equals to the file.
            fprintf(writeFile, "%s=", levelName[level - 1]);
            // Printing the return value but leaving last 2 chars which are ';' and end char.
            for(int i = 1; i < strlen(returnValue) - 2; i++)
                fprintf(writeFile, "%c", returnValue[i]);
            // Printing new line.
            fprintf(writeFile, "\n");
            break;
        }
        

    case ASSIGNMENT:

        {for(int i = 0; i < strlen(line); i++){
            if(line[i] == '='){ // If an equals is found then we need to find the assignments left and right variables.
                
                // Defining necessary variables.
                int j = i;
                int k = i;
                int equalsIndex = i;

                char leftVariableName[MAX_LINE_LENGTH] = {0};
                char rightVariableName[MAX_LINE_LENGTH] = {0};

                // Until you find a ' ' or ',' that is not 1 index away from '=' to the left. Or you reach to the beginning. 
                while(!((line[j] == ' ' || line[j] == ',' || j == 0) && equalsIndex - j != 1)){
                    j--;
                }
                
                int leftVariableNameBeginIndex = 0;
                if(j != 0) // If j == 0 then the variable name begins directly. Therefore no need to increment.
                    leftVariableNameBeginIndex = j + 1;

                // Copying left variable name
                strncpy(leftVariableName, line + leftVariableNameBeginIndex, equalsIndex - leftVariableNameBeginIndex);
                    
                // Indentation
                for(int i = 0; i < level; i++)
                    fprintf(writeFile, "\t");

                // Write left variable name and the equals.
                fprintf(writeFile ,"%s=", leftVariableName);

                // Until you find a ' ' or ',' or ';' that is not 1 index away from '=' to the right. Or you reach to the end.
                while(!((line[k] == ' ' || line[k] == ',' || line[k] == ';' || k == strlen(line) - 1) && k - equalsIndex != 1)){
                    if(line[k] == '('){ // If you find opening parenthesis then this is a function call and k must be at the end.
                        k = strlen(line) - 1;
                        break;
                    }
                    k++;
                }
            
                // Right variable name end index is set to k.
                int rightVariableNameEndIndex = k;

                // Copying right variable name. 
                strncpy(rightVariableName, line + equalsIndex + 1, rightVariableNameEndIndex - equalsIndex -1);
                
                // If the right side of the operation is a function call then write to file accordingly
                if(findOperation(rightVariableName, level) == FUNCTION_CALL){
                    writeToFile(FUNCTION_CALL, rightVariableName, 0); // This is called with level 0 in order to stop it from indenting
                                                                      // Since indentation is already done.
                } else {
                    fprintf(writeFile, "%s\n", rightVariableName); // Else just print the right variable name.
                }

                
            }
        }

        break;}



    }
}

/*
    Resets levelName in order to prevent memory errors.
*/
void resetLevelName(int level){
    for(int i = 0; i < strlen(levelName[level]); i++){
        levelName[level][i] = '\0';
    }
}

/*
    Resets step in order to prevent memory errors.
*/
void resetStep(){
    for(int i = 0; i < strlen(step); i++){
        step[i] = '\0';
    }
}
