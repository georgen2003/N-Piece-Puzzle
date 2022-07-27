#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "8piece_puzzle.h"

//void expand(State* cur_state, PQ Queue, ListPtr Visited);
void expand(State* cur_state, PQ Queue,RB tree);
State* CopyState(State* cur_state);
ListPtr ReturnSolution(State* state);
char* IntToAscii(int number);

ListPtr solve(State* state, PQ Queue, RB tree)
{
    State* cur_state = state;
    while(isGoal(*cur_state) != 1)
    {
        expand(cur_state,Queue,tree);

        cur_state = (State*)PQ_Pop(Queue);

        RB_InsertKey(tree,cur_state);
    }

    ListPtr move_list = ReturnSolution(cur_state);
        
    return move_list;
}

void expand(State* cur_state, PQ Queue,RB tree)
{
    int size = cur_state->size;

    int RowOffset[]={-1,0,+1,0};
    int ColOffset[]={0,-1,0,+1};

    int new_col;
    int new_row;

    State* new_state; 

    int i;
    for(i=0; i<4; i++)
    {
        new_col = cur_state->blank_col + ColOffset[i];
        new_row = cur_state->blank_row + RowOffset[i];

        if(new_col >= 0 && new_col < size && new_row >= 0 && new_row < size /* && (cur_state->parent == NULL || (new_col != cur_state->parent->blank_col && new_row != cur_state->parent->blank_row)) */ )
        {
            new_state = CopyState(cur_state);   
            new_state->board[cur_state->blank_row][cur_state->blank_col] = cur_state->board[new_row][new_col];
            new_state->board[new_row][new_col]=0;
            new_state->blank_row = new_row;
            new_state->blank_col = new_col;
            
            evaluate(new_state);

            if(RB_Search(tree,new_state) ==  NULL)
            {
                PQ_Insert(Queue, new_state);
            }
            else
            {
                destroyfunc(new_state);
            }
        }
    }
}

State* CopyState(State* cur_state)
{
    State* new_state =  malloc(sizeof(State));
    new_state->parent = cur_state;
    new_state->moves = cur_state->moves+1;
    new_state->size = cur_state->size;


    new_state->board = malloc(sizeof(int*) * new_state->size);
    for(int i=0; i<new_state->size; i++)
        new_state->board[i] = malloc(sizeof(int) * new_state->size);

    for(int i=0; i<new_state->size; i++)
        for(int j=0; j<new_state->size; j++)
            new_state->board[i][j] = cur_state->board[i][j];

    return new_state;        
}

int compare(Pointer a, Pointer b)
{
    State* state_a = (State*)a;
    State* state_b = (State*)b;

    return strcmp(state_a->representation,state_b->representation);
}

void destroyfunc(Pointer ptr)
{
    State* a = (State*)ptr;

    if(a == NULL)
        return;

    for(int i=0; i<a->size; i++)
        free(a->board[i]);

    free(a->board);

    free(a);
}

int compare_evals(Pointer first,Pointer second)
{
    State* a = (State*)first;
    State* b = (State*)second;

    return b->eval - a->eval;
}

void evaluate (State* state)
{
    int str_size = 0;
    int temp = state->size * state->size;
    if(temp < 10)
    {
        str_size = temp;
    }
    else
    {
        str_size = 10;
        temp -= 10;
        temp *= 2;
        str_size += temp;
    }
    state->representation = malloc(sizeof(char) * str_size + 1);
    state->representation[str_size] = '\0';
    state->representation[0] = '\0';

    int size = state->size;
    int mScore=0,counter=1;
    int i,j;
    for (i=0; i<size; i++)
    {
        for(j=0; j<size; j++)
        {
            char* temp_str = IntToAscii(state->board[i][j]);
            strcat(state->representation,temp_str);
            free(temp_str);
            
            if(state->board[i][j] == 0)
            {
                state->blank_row = i;
                state->blank_col = j;
            }

            if(state->board[i][j] != 0)
            {
                if(state->board[i][j] != counter)
                {
                    int cur = state->board[i][j] - 1;
                    int wRow = floor(cur / size);
                    int wCol = floor(cur % size);

                    mScore += abs(i - wRow);
                    mScore += abs(j - wCol);
                }
            }
            if(counter == (size*size)-1
            )
                counter = 0;
            else    
                counter++;
        }
    }

    state->eval = mScore + state->moves;
    return;
}

int isGoal(State state)
{
    int size = state.size;
    int counter=1;
    int i,j;
    for(i=0; i<size; i++)
        for(j=0; j<size; j++)
        {
            int current = state.board[i][j];
            if(current != counter)
                return 0;
            
            if(counter == (size*size)-1)
                counter = 0;
            else    
                counter++;
        }
    
    return 1;
}

ListPtr ReturnSolution(State* state)
{
    ListPtr list = CreateList(NULL,NULL);
    while(state != NULL)
    {
        ListInsert(state,list);
        
        state = state->parent;
    }
    return list;
}

char* IntToAscii(int number)
{
    if(number == 0)
    {
        char* str = malloc(sizeof(char) * 2);
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    int size = 0;
    int temp = number;
    while(temp != 0)
    {
        temp /= 10;
        size++;
    }
    char* str = malloc(sizeof(char) * size);
    int i = 0;
    while(i < size)
    {
        str[size - i - 1] = number % 10 + '0';
        number /= 10;
        i++;
    }
    str[i] = '\0';
    return str;
}

int IsSolveable(State* state)
{
    int size = state->size;
    int inv = 0;

    int temp[state->size * state->size];
    int index = 0;

    for(int i=0; i < size; i++)
        for(int j=0; j < size; j++)
            temp[index++] = state->board[i][j];

    for(int i=0; i < size * size - 1; i++)
    {   
        if(temp[i] == 0) continue;

        for(int j=i + 1; j < size * size; j++)
        {
            if(temp[j] == 0) continue;
            if (temp[j] && temp[i] && temp[i] > temp[j]) inv++;
        }
    }

    if(inv % 2 == 1)
        return 0;
    else
        return 1;
}