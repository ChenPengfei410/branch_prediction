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

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define convert(c) (c == SN || c == WN) ? NOTTAKEN : TAKEN;

//
// TODO:Student Information
//
const char *studentName = "Pengfei Chen";
const char *studentID   = "A53309067";
const char *email       = "jackchan0410gmail.com";
//2

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
    case CUSTOM:
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
  switch (bpType) {
    case GSHARE:
      return gshare_train(pc, outcome);
    case TOURNAMENT:
      return tournament_train(pc, outcome);
    case CUSTOM:
    default:
      break;
  }
}
