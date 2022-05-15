//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Sadhana Shanmuga Sundaram";
const char *studentID   = "A59011077";
const char *email       = "sshanmugasundaram@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here. 
int ghistoryBits = 14; // Number of bits used for Global History
int bpType;       // Branch Prediction Type
int verbose;


//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
uint8_t *bht_gshare;
uint64_t ghistory;

uint8_t *tournament_bht_gp;
uint8_t *tournament_bht_lp;
uint16_t *tournament_lht;
uint8_t *tournament_ct;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
 int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t*)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_entries; i++){
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}



uint8_t 
gshare_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch(bht_gshare[index]){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
      return NOTTAKEN;
  }
}

void
train_gshare(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  //Update state of entry in bht based on outcome
  switch(bht_gshare[index]){
    case WN:
      bht_gshare[index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      bht_gshare[index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      bht_gshare[index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      bht_gshare[index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome); 
}

void
cleanup_gshare() {
  free(bht_gshare);
}

////////Tournament Predictor////////////
int tournament_gp_len = 12;
int tournament_choice_len = 12;
int tournament_lht_len = 12;
int tournament_lp_len = 12;

void init_tournament(){
  int bht_gp_entries = 1 << tournament_gp_len;
  tournament_bht_gp = (uint8_t*)malloc(bht_gp_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_gp_entries; i++){
    tournament_bht_gp[i] = WT;
    //printf("Tournament BHT GP %d : %d \n",i,tournament_bht_gp[i]);
  }
  ghistory = 0;

  int ct_entries = 1 << tournament_choice_len;
  tournament_ct = (uint8_t*)malloc(ct_entries * sizeof(uint8_t));
  for(i = 0; i< ct_entries; i++){
    tournament_ct[i] = WT;
    //printf("Tournament Choice Table %d : %d \n",i,tournament_ct[i]);
  }

  int lht_entries = 1 << tournament_lht_len;
  tournament_lht = (uint16_t*)malloc(lht_entries * sizeof(uint16_t));
  for(i = 0; i< lht_entries; i++){
    tournament_lht[i] = 0;
    //printf("Tournament LHT %d : %d \n",i,tournament_lht[i]);
  }
  
  int bht_lp_entries = 1 << tournament_lp_len;
  tournament_bht_lp = (uint8_t*)malloc(bht_lp_entries * sizeof(uint8_t));
  for(i = 0; i< bht_lp_entries; i++){
    tournament_bht_lp[i] = WT;
    //printf("Tournament BHT LP %d : %d \n",i,tournament_bht_lp[i]);
  }

}

uint8_t tournament_predict(uint32_t pc){
  uint32_t bht_gp_entries = 1 << tournament_gp_len;
  uint32_t index_ght_ct = ghistory & (bht_gp_entries -1);


  uint32_t lht_entries = 1 << tournament_lht_len;

  uint32_t lp_pc_lower_bits = pc & (lht_entries-1);
  uint16_t index_pht = tournament_lht[lp_pc_lower_bits];

  int cp_result;


  switch(tournament_ct[index_ght_ct]){
    case WN:
      cp_result = 0;
      break;
    case SN:
      cp_result = 0;
      break;
    case WT:
      cp_result = 1;
      break;
    case ST:
      cp_result = 1;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Tournament Choice Table %d %d!\n", index_ght_ct, tournament_ct[index_ght_ct]);
      cp_result = 0;
  }

  uint8_t bp_result;

  if(cp_result == 0){
    bp_result = tournament_bht_lp[index_pht];
  }
  else {
    bp_result = tournament_bht_gp[index_ght_ct];
  }
  switch(bp_result){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      if(cp_result == 0)
        printf("Warning: PREDICT : Undefined state of entry in Tournament Local BHT! %d %d \n",index_pht,tournament_bht_lp[index_pht]);
      else
        printf("Warning: PREDICT : Undefined state of entry in Tournament Global BHT! %d %d \n",index_ght_ct,tournament_bht_gp[index_ght_ct]);
      return NOTTAKEN;
  }
}

void train_tournament(uint32_t pc, uint8_t outcome){
  uint32_t bht_gp_entries = 1 << tournament_gp_len;
  uint32_t index_ght_ct = ghistory & (bht_gp_entries -1);


  uint32_t lht_entries = 1 << tournament_lht_len;

  uint32_t lp_pc_lower_bits = pc & (lht_entries-1);
  uint16_t index_pht = tournament_lht[lp_pc_lower_bits];

  int cp_result;

  switch(tournament_ct[index_ght_ct]){
    case WN:
      cp_result = 0;
      break;
    case SN:
      cp_result = 0;
      break;
    case WT:
      cp_result = 1;
      break;
    case ST:
      cp_result = 1;
      break;
    default:
      printf("Warning: Undefined state of entry in Tournament Choice Table!\n");
      cp_result = 0;
  }

  uint8_t bp_result;
  bool mispredict = true;

  if(cp_result == 0){
    bp_result = tournament_bht_lp[index_pht];
  }
  else {
    bp_result = tournament_bht_gp[index_ght_ct];
  }
  if((bp_result == WN || bp_result == SN) && (outcome == NOTTAKEN))
    mispredict = false;
  else if((bp_result == WT || bp_result == ST) && (outcome == TAKEN))
    mispredict = false;
/*
  switch(bp_result){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      if(cp_result == 0)
        printf("Warning: Undefined state of entry in Tournament Local BHT!\n");
      else
        printf("Warning: Undefined state of entry in Tournament Global BHT!\n");
      return NOTTAKEN;
  }
*/
  bool lp_correct = false;
  bool gp_correct = false; 
  if((tournament_bht_lp[index_pht] == WN || tournament_bht_lp[index_pht] == SN) && (outcome == NOTTAKEN))
    lp_correct = true;
  else if((tournament_bht_lp[index_pht] == WT || tournament_bht_lp[index_pht] == ST) && (outcome == TAKEN))
    lp_correct = true;

  if((tournament_bht_gp[index_ght_ct] == WN || tournament_bht_gp[index_ght_ct] == SN) && (outcome == NOTTAKEN))
    lp_correct = true;
  else if((tournament_bht_gp[index_ght_ct] == WT || tournament_bht_gp[index_ght_ct] == ST) && (outcome == TAKEN))
    lp_correct = true;

  if(mispredict){
    if(cp_result == 0 && !lp_correct && gp_correct || cp_result == 1 && lp_correct && !gp_correct){
      switch(tournament_ct[index_ght_ct]){
      case WN:
        tournament_ct[index_ght_ct] = WT;
        break;
      case SN:
        tournament_ct[index_ght_ct] = WN;
        break;
      case WT:
        tournament_ct[index_ght_ct] = WN;
        break;
      case ST:
        tournament_ct[index_ght_ct] = WT;
        break;
      default:
        printf("Warning: Undefined state of entry in Choice table!\n");
      }
    }
    if(!lp_correct){
      switch(tournament_bht_lp[index_pht]){
      case WN:
        tournament_bht_lp[index_pht] = (outcome==TAKEN)?WT:SN;
        break;
      case SN:
        tournament_bht_lp[index_pht] = (outcome==TAKEN)?WN:SN;
        break;
      case WT:
        tournament_bht_lp[index_pht] = (outcome==TAKEN)?ST:WN;
        break;
      case ST:
        tournament_bht_lp[index_pht] = (outcome==TAKEN)?ST:WT;
        break;
      default:
        printf("Warning: Undefined state of entry in LP table!\n");
      }
    }
    if(!gp_correct){
      switch(tournament_bht_gp[index_ght_ct]){
      case WN:
        tournament_bht_gp[index_ght_ct] = (outcome==TAKEN)?WT:SN;
        break;
      case SN:
        tournament_bht_gp[index_ght_ct] = (outcome==TAKEN)?WN:SN;
        break;
      case WT:
        tournament_bht_gp[index_ght_ct] = (outcome==TAKEN)?ST:WN;
        break;
      case ST:
        tournament_bht_gp[index_ght_ct] = (outcome==TAKEN)?ST:WT;
        break;
      default:
        printf("Warning: Undefined state of entry in GP table!\n");
      }
    }
  }

  ghistory = ((ghistory << 1) | outcome); 
  tournament_lht[lp_pc_lower_bits] = ((tournament_lht[lp_pc_lower_bits] << 1) | outcome) & (lht_entries-1);

}

///////////////////////////////////////







void
init_predictor()
{
  switch (bpType) {
    case STATIC:
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

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
      return tournament_predict(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
    default:
      break;
  }
  

}
