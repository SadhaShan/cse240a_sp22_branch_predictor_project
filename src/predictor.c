//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <stdint.h>
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
//
//gshare
int tournament_gp_len = 11;
int tournament_choice_len = 11;
int tournament_lht_len = 10;
int tournament_lp_len = 10;

uint8_t *bht_gshare;
uint64_t ghistory;

uint8_t *tournament_bht_gp;
uint8_t *tournament_bht_lp;
uint16_t *tournament_lht;
uint8_t *tournament_ct;

int num_perceptrons = 255;
int perceptron_history_len = 15;
int perceptron_train_threshold;
int16_t *perceptron_table;



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

void init_tournament(){
  int bht_gp_entries = 1 << tournament_gp_len;
  tournament_bht_gp = (uint8_t*)malloc(bht_gp_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_gp_entries; i++){
    tournament_bht_gp[i] = WN;
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
    tournament_bht_lp[i] = WN;
    //printf("Tournament BHT LP %d : %d \n",i,tournament_bht_lp[i]);
  }

}

uint8_t tournament_predict(uint32_t pc){
  uint32_t bht_gp_entries = 1 << tournament_gp_len;
  uint32_t index_ght_ct = ghistory & (bht_gp_entries -1);


  uint32_t lht_entries = 1 << tournament_lht_len;

  uint32_t lp_pc_lower_bits = pc & (lht_entries-1);
  uint16_t index_pht = tournament_lht[lp_pc_lower_bits];

  uint8_t lp_predict;
  switch(tournament_bht_lp[index_pht]){
    case WN:
      lp_predict = NOTTAKEN;
      break;
    case SN:
      lp_predict = NOTTAKEN;
      break;
    case WT:
      lp_predict = TAKEN;
      break;
    case ST:
      lp_predict = TAKEN;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Tournament Local BHT! %d %d \n",index_pht,tournament_bht_lp[index_pht]);
      lp_predict = NOTTAKEN;
  }
  
  uint8_t gp_predict;
  switch(tournament_bht_gp[index_ght_ct]){
    case WN:
      gp_predict = NOTTAKEN;
      break;
    case SN:
      gp_predict = NOTTAKEN;
      break;
    case WT:
      gp_predict = TAKEN;
      break;
    case ST:
      gp_predict = TAKEN;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Tournament Globql BHT! %d %d \n",index_ght_ct,tournament_bht_gp[index_ght_ct]);
      gp_predict = NOTTAKEN;
  }

  if(gp_predict == lp_predict)
    return gp_predict;
  else{
    uint8_t ct_predict;
    switch(tournament_ct[index_ght_ct]){
    case WN:
      ct_predict = 0;
      break;
    case SN:
      ct_predict = 0;
      break;
    case WT:
      ct_predict = 1;
      break;
    case ST:
      ct_predict = 1;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Choice Table! %d %d \n",index_ght_ct,tournament_ct[index_ght_ct]);
      ct_predict = 1;
    }
  
    if(ct_predict == 0)
      return lp_predict;
    else
      return gp_predict;
  }


}

void train_tournament(uint32_t pc, uint8_t outcome){
  uint32_t bht_gp_entries = 1 << tournament_gp_len;
  uint32_t index_ght_ct = ghistory & (bht_gp_entries -1);


  uint32_t lht_entries = 1 << tournament_lht_len;

  uint32_t lp_pc_lower_bits = pc & (lht_entries-1);
  uint16_t index_pht = tournament_lht[lp_pc_lower_bits];

  uint8_t lp_predict;
  switch(tournament_bht_lp[index_pht]){
    case WN:
      lp_predict = NOTTAKEN;
      break;
    case SN:
      lp_predict = NOTTAKEN;
      break;
    case WT:
      lp_predict = TAKEN;
      break;
    case ST:
      lp_predict = TAKEN;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Tournament Local BHT! %d %d \n",index_pht,tournament_bht_lp[index_pht]);
      lp_predict = NOTTAKEN;
  }
  
  uint8_t gp_predict;
  switch(tournament_bht_gp[index_ght_ct]){
    case WN:
      gp_predict = NOTTAKEN;
      break;
    case SN:
      gp_predict = NOTTAKEN;
      break;
    case WT:
      gp_predict = TAKEN;
      break;
    case ST:
      gp_predict = TAKEN;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Tournament Globql BHT! %d %d \n",index_ght_ct,tournament_bht_gp[index_ght_ct]);
      gp_predict = NOTTAKEN;
  }

  uint8_t ct_predict;
  uint8_t bp_result;

  if(gp_predict == lp_predict)
    bp_result = gp_predict;
  else{
    switch(tournament_ct[index_ght_ct]){
    case WN:
      ct_predict = 0;
      break;
    case SN:
      ct_predict = 0;
      break;
    case WT:
      ct_predict = 1;
      break;
    case ST:
      ct_predict = 1;
      break;
    default:
      printf("Warning: PREDICT : Undefined state of entry in Choice Table! %d %d \n",index_ght_ct,tournament_ct[index_ght_ct]);
      ct_predict = 1;
    }
  
    if(ct_predict == 0)
      bp_result = lp_predict;
    else
      bp_result = gp_predict;
  }
  
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

    if(ct_predict == 0 && !lp_correct && gp_correct || ct_predict == 1 && lp_correct && !gp_correct){
      uint8_t ct_prediction = tournament_ct[index_ght_ct];
      switch((gp_predict << 1)| lp_predict){
      case WN:
        if(lp_correct)
          if(ct_prediction != SN)
            tournament_ct[index_ght_ct] -= 1;
        else
          if(ct_prediction != ST)
            tournament_ct[index_ght_ct] += 1; 
        break;
      case SN:
        break;
      case WT:
        if(lp_correct)
          if(ct_prediction != SN)
            tournament_ct[index_ght_ct] -= 1;
        else
          if(ct_prediction != ST)
            tournament_ct[index_ght_ct] += 1; 
        break;
      case ST:
        break;
      default:
        printf("Warning: Undefined state of entry in Choice table!\n");
      }
    }

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

  ghistory = ((ghistory << 1) | outcome) & (bht_gp_entries - 1); 
  tournament_lht[lp_pc_lower_bits] = ((tournament_lht[lp_pc_lower_bits] << 1) | outcome) & (lht_entries-1);

  //printf("AFTER TRAIN : %x %d %d LP %d %d %d ; GP %d %d %d; LHT %d %x; GH %lu \n",pc,outcome,bp_result,lp_correct,index_pht,tournament_bht_lp[index_pht],gp_correct,index_ght_ct,tournament_bht_gp[index_ght_ct],lp_pc_lower_bits,tournament_ct[lp_pc_lower_bits],ghistory);

}

///////////////////////////////////////
/*
int num_perceptrons = 170;
int perceptron_history_len = 23;
int perceptron_train_threshold;
int8_t *perceptron_table;
*/

/////////Perceptron Predictor//////////

void init_perceptron(){
  int perceptron_table_entries = num_perceptrons*(perceptron_history_len+1);
  perceptron_table = (int16_t*)malloc(perceptron_table_entries*sizeof(int16_t));
  int i=0;
  for(i=0; i < perceptron_table_entries; i=i+1){
    perceptron_table[i] = 0;
  }
  perceptron_train_threshold = (int)(1.93*perceptron_history_len + 14);
  ghistory = 0;
}

uint8_t perceptron_predict(uint32_t pc){
  uint32_t table_index = (pc % num_perceptrons) * (perceptron_history_len+1);
  int16_t y = perceptron_table[table_index];
  uint64_t curr_ghistory = ghistory;
  for(int i=1; i<=perceptron_history_len; i=i+1){
    if(curr_ghistory&1)
      y += perceptron_table[table_index+i];
    else
      y -= perceptron_table[table_index+i];
    curr_ghistory = curr_ghistory >> 1;
  }
  if(y<0)
    return NOTTAKEN;
  else
    return TAKEN;
}

void train_perceptron(uint32_t pc, uint8_t outcome){
  uint32_t table_index = (pc % num_perceptrons) * (perceptron_history_len+1);
  int16_t y = perceptron_table[table_index];
  uint64_t curr_ghistory = ghistory;
  for(int i=1; i<=perceptron_history_len; i=i+1){
    if(curr_ghistory&1)
      y += perceptron_table[table_index+i];
    else
      y -= perceptron_table[table_index+i];
    curr_ghistory = curr_ghistory >> 1;
  }
  uint8_t bp_result;
  if(y<0)
    bp_result = NOTTAKEN;
  else
    bp_result = TAKEN;
  
  bool mispredict = true;
  if(bp_result == outcome)
    mispredict = false;
  if(mispredict || abs(y) <= perceptron_train_threshold){
    if(outcome == 1){
      if(abs(perceptron_table[table_index]+1) < perceptron_train_threshold)
        perceptron_table[table_index] ++;
    }
    else{
      if(abs(perceptron_table[table_index]-1) < perceptron_train_threshold)
        perceptron_table[table_index] --;
    }
    uint64_t curr_ghistory = ghistory;
    for(int i=1; i<=perceptron_history_len; i=i+1){
      if(outcome == (curr_ghistory&1)){
        if(abs(perceptron_table[table_index+i]+1) < perceptron_train_threshold)
          perceptron_table[table_index+i] += 1;
      }
      else {
        if(abs(perceptron_table[table_index+i]-1) < perceptron_train_threshold)
          perceptron_table[table_index+i] -= 1;
      }
      curr_ghistory = curr_ghistory >> 1;
    }
  }
  //if(abs(y)>perceptron_train_threshold)
  //  printf("Output threshold crossed! %x %d %d \n",pc,y,perceptron_train_threshold);
  
  ghistory = ((ghistory << 1) | outcome);
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
      init_perceptron();
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
      return perceptron_predict(pc);
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
      return train_perceptron(pc, outcome);
    default:
      break;
  }
  

}
