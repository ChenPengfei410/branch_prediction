//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

#define abs(a) ((a) < 0 ? -(a) : (a))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define convert(c) (c == SN || c == WN) ? NOTTAKEN : TAKEN;

//
// TODO:Student Information
//
const char *studentName = "Pengfei Chen";
const char *studentID   = "A53309067";
const char *email       = "pec003@eng.ucsd.edu";
const char *studentName = "Kaichen Tang";
const char *studentID   = "A53303810";
const char *email       = "k4tang@eng.ucsd.edu";


//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

int N;
int trained[20] = {0, 6, 0, 2, 3, 3, 0, 1, -1, 0, 0, 1, 1, 0, 0, 0, 0, 0, -1, 0};

uint32_t globalhistory;

uint8_t *gshare_BHT;
uint32_t gshare_BHT_size;

uint32_t *localPHT;
uint8_t *localBHT;
uint8_t *globalBHT;
uint8_t *choicePT;
uint32_t localBHT_size;
uint32_t localPHT_size;
uint32_t choicePT_size;
uint32_t globalBHT_size;
uint8_t tournament_localPre;
uint8_t tournament_globalPre;

//perceptron
int p_ghistoryBits = 29;
int tableSize = 60;
//int p_ghistoryBits = 19;
//int tableSize = 100;
int theta = 1.93 * 29 + 14; // Threshold of perceptron
//int theta = 17;
uint32_t p_mask;
uint32_t p_globalReg;   //global pattern

uint8_t *p_PHT;
int **perceptronTable; //pointer to tables
uint8_t *p_choicePT;
int p_score, id;
uint8_t p_prediction;



//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void init_gshare() {
  gshare_BHT_size = 1 << ghistoryBits;
  gshare_BHT = malloc(gshare_BHT_size * sizeof(uint8_t));
  memset(gshare_BHT, WN, gshare_BHT_size * sizeof(uint8_t));
}


void init_tournament() {
    globalhistory = 0;
    
    localBHT_size= (1 << lhistoryBits) * sizeof(uint8_t);
    localPHT_size= (1 << pcIndexBits) * sizeof(uint32_t);
    choicePT_size= (1 << ghistoryBits) * sizeof(uint8_t);
    globalBHT_size= choicePT_size ;
    
    
    localBHT = malloc(localBHT_size);
    localPHT = malloc(localPHT_size);
    choicePT = malloc(choicePT_size);
    globalBHT = malloc(globalBHT_size);
    
    memset(localBHT, WN, localBHT_size);
    memset(localPHT, 0, localPHT_size);
    memset(choicePT, WN, choicePT_size);
    memset(globalBHT, WN, globalBHT_size);
}

//void init_table(){
//    FILE *fp = NULL;
//    fp = fopen("6.txt","r");
//    for (int i = 0; i < tableSize; ++i) {
//      for (int j = 0; j <= p_ghistoryBits; ++j) {
//
//        fscanf(fp,"%d\t",&perceptronTable[i][j]);
//        printf("%d\t",perceptronTable[i][j]);
//      }
//        fgetc(fp);
//    }
//}

void init_perceptron()
{
    p_mask = (1 << p_ghistoryBits) - 1;
    
    int p_choicePT_size = (1 << p_ghistoryBits) * sizeof(uint8_t);
    p_choicePT = malloc(p_choicePT_size);
    memset(p_choicePT, WN, p_choicePT_size);   // default as w not taken
    
    int p_PHT_size = (1 << p_ghistoryBits) * sizeof(uint8_t);
    p_PHT = malloc(p_PHT_size);
    memset(p_PHT, WN, p_PHT_size);
    
    int perceptronTable_size = tableSize * sizeof(int*);
    perceptronTable = (int **) malloc(perceptronTable_size);
    int i = 0;
    int tableElemSize = (p_ghistoryBits + 1) * sizeof(int);
    while(i < tableSize)
    {
        perceptronTable[i] = (int *) malloc(tableElemSize);
        memset(perceptronTable[i], 0, tableElemSize);
        //for (int j = 0; j < 20; ++j)
        //  perceptronTable[i][j] = trained[j];
        perceptronTable[i][0] = 1;
        i++;
    }
    //init_table();

}



// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //

  // Do initialization based on the bpType
  switch (bpType) {
    case GSHARE:
          init_gshare();
          break;
    case TOURNAMENT:
          init_tournament();
          break;
    case CUSTOM:
          init_perceptron();
          break;
    default:
      break;
  }
}

uint8_t
gshare_predict(uint32_t pc) {
  uint32_t index = pc ^ globalhistory;
  return convert(gshare_BHT[index & (gshare_BHT_size - 1)]);
}

uint8_t tournament_prediction(uint32_t pc) {
    uint32_t gsize = ((1 << ghistoryBits) - 1);
    uint32_t lsize = ((1 << pcIndexBits) - 1);
    uint32_t predictor = choicePT[ gsize & globalhistory ];
    
    //global
    uint32_t gBHTIndex = globalhistory & gsize;
    if (globalBHT[gBHTIndex] == WN || globalBHT[gBHTIndex] == SN)
        tournament_globalPre = NOTTAKEN;
    else
        tournament_globalPre = TAKEN;
    
    //local
    uint32_t localPHTIndex = lsize & pc;
    uint32_t localBHTIndex = localPHT[localPHTIndex];
    if (localBHT[localBHTIndex] == WN || localBHT[localBHTIndex] == SN)
        tournament_localPre = NOTTAKEN;
    else
        tournament_localPre = TAKEN;
    
    return ((predictor == WN || predictor == SN) ? tournament_globalPre : tournament_localPre);
    
}

uint8_t perceptron_prediction(uint32_t pc)
{
    id = pc % tableSize;
    int *allW = perceptronTable[id];
    p_score = allW[0];
    uint32_t history = p_globalReg;
    
    int b, i=0;
    while(i < p_ghistoryBits){
        b = history % 2;
        history /= 2;
        p_score = ((b == 0) ? (p_score - allW[i + 1]) : (p_score + allW[i + 1]));
        ++i;
    }
    
    return ((p_score < 0)? NOTTAKEN:TAKEN);

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
      return tournament_prediction(pc);
    case CUSTOM: {
      p_prediction = perceptron_prediction(pc);
      return p_prediction;
    }
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

void state_update(uint8_t* state, uint8_t outcome) {
  if (outcome == TAKEN)
    *state = min(*state + 1, ST);
  else
    *state = max(*state - 1, SN);
}

void
gshare_train(uint32_t pc, uint8_t outcome) {
  uint32_t index = pc ^ globalhistory;
  state_update(&gshare_BHT[index & (gshare_BHT_size - 1)], outcome); 

  globalhistory <<= 1;
  globalhistory += outcome;
}

void
tournament_train(uint32_t pc, uint8_t outcome) {

    if (tournament_globalPre != tournament_localPre)
    {
        uint8_t ioutcome;
        uint8_t* istate = & choicePT[globalhistory];
        if (tournament_localPre == outcome)
            ioutcome = TAKEN;
        else
            ioutcome = NOTTAKEN;
        state_update(istate,ioutcome);
    }
    
    uint32_t lPHTsize = ((1 << pcIndexBits) - 1);
    uint32_t localPHTIndex = lPHTsize & pc;
    uint32_t localBHTIndex = localPHT[localPHTIndex];

    state_update(&(localBHT[localBHTIndex]), outcome);
    state_update(&globalBHT[globalhistory], outcome);
    
    uint32_t a = ((1 << lhistoryBits) - 1);
    uint32_t b = ((1 << ghistoryBits) - 1);
    localPHT[localPHTIndex] <<= 1;
    globalhistory <<= 1;
    localPHT[localPHTIndex] &= a;
    globalhistory &= b;
    
    localPHT[localPHTIndex] |= outcome;
    globalhistory |= outcome;
}

void
perceptron_train(uint32_t pc, uint8_t outcome) {
  if (p_prediction != outcome || abs(p_score) < theta) {
      
    int range=1;
    if (p_prediction != outcome)
    {
        if (abs(p_score) > theta )
            range=4;
        else
            range=3;
    }
    else
        range=2;
    
    int *allW = perceptronTable[id];
    allW[0] += (outcome == TAKEN) ? range : -range;

    uint32_t history = p_globalReg;
    for (int i = 1; i <= p_ghistoryBits; ++i) {
      allW[i] += ((history % 2) == outcome) ? range : -range;
      history /= 2;
    }
  }

  p_globalReg <<= 1;
  p_globalReg += outcome;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  ++N;


  if (N==10000)
  {
    for (int i = 0; i < tableSize; ++i) {
      for (int j = 0; j <= p_ghistoryBits; ++j) {
        //weights[j] += perceptronTable[i][j];
        printf("%d\t",perceptronTable[i][j]);
      }
        printf("\n");
    }
  }
  
  switch (bpType) {
    case GSHARE:
      return gshare_train(pc, outcome);
    case TOURNAMENT:
      return tournament_train(pc, outcome);
    case CUSTOM:
      return perceptron_train(pc, outcome);
    default:
      break;
  }
}
