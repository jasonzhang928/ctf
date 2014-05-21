/*Copyright (c) 2013, Edgar Solomonik, all rights reserved.*/

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
//#include "../shared/util.h"
//#include "../../include/ctf.hpp"
#include "ctf_expression.h"
#include "ctf_schedule.h"

//template<typename dtype>
//CTF_Idx_Tensor<dtype> get_intermediate(CTF_Idx_Tensor<dtype>& A, 
//                                        CTF_Idx_Tensor<dtype>& B){
//  int * len_C, * sym_C;
//  char * idx_C;
//  int ndim_C, i, j, idx;
//  
//  ndim_C = 0;
//  for (i=0; i<A.parent->ndim; i++){
//    ndim_C++;
//    for (j=0; j<B.parent->ndim; j++){
//      if (A.idx_map[i] == B.idx_map[j]){
//        ndim_C--;
//        break;
//      }
//    }
//  }
//  for (j=0; j<B.parent->ndim; j++){
//    ndim_C++;
//    for (i=0; i<A.parent->ndim; i++){
//      if (A.idx_map[i] == B.idx_map[j]){
//        ndim_C--;
//        break;
//      }
//    }
//  }
//
//  idx_C = (char*)CTF_alloc(sizeof(char)*ndim_C);
//  sym_C = (int*)CTF_alloc(sizeof(int)*ndim_C);
//  len_C = (int*)CTF_alloc(sizeof(int)*ndim_C);
//  idx = 0;
//  for (i=0; i<A.parent->ndim; i++){
//    for (j=0; j<B.parent->ndim; j++){
//      if (A.idx_map[i] == B.idx_map[j]){
//        break;
//      }
//    }
//    if (j == B.parent->ndim){
//      idx_C[idx] = A.idx_map[i];
//      len_C[idx] = A.parent->len[i];
//      if (idx >= 1 && i >= 1 && idx_C[idx-1] == A.idx_map[i-1] && A.parent->sym[i-1] != NS){
//        sym_C[idx-1] = A.parent->sym[i-1];
//      }
//      sym_C[idx] = NS;
//      idx++;
//    }
//  }
//  for (j=0; j<B.parent->ndim; j++){
//    for (i=0; i<A.parent->ndim; i++){
//      if (A.idx_map[i] == B.idx_map[j]){
//        break;
//      }
//    }
//    if (i == A.parent->ndim){
//      idx_C[idx] = B.idx_map[j];
//      len_C[idx] = B.parent->len[j];
//      if (idx >= 1 && j >= 1 && idx_C[idx-1] == B.idx_map[j-1] && B.parent->sym[j-1] != NS){
//        sym_C[idx-1] = B.parent->sym[j-1];
//      }
//      sym_C[idx] = NS;
//      idx++;
//    }
//  }
//
//  CTF_Tensor<dtype> * tsr_C = new CTF_Tensor<dtype>(ndim_C, len_C, sym_C, *(A.parent->world));
//  CTF_Idx_Tensor<dtype> out(tsr_C, idx_C);
//  out.is_intm = 1;
//  CTF_free(sym_C);
//  CTF_free(len_C);
//  return out;
//}

template<typename dtype>
CTF_Idx_Tensor<dtype>::CTF_Idx_Tensor(CTF_Tensor<dtype> *  parent_, 
                                        const char *          idx_map_, 
                                        int                   copy){
  if (copy){
    parent = new CTF_Tensor<dtype>(*parent,1);
    idx_map = (char*)CTF_alloc(parent->ndim*sizeof(char));
  } else {
    idx_map = (char*)CTF_alloc(parent_->ndim*sizeof(char));
    parent        = parent_;
  }
  memcpy(idx_map, idx_map_, parent->ndim*sizeof(char));
  is_intm       = 0;
  this->scale    = 1.0;
}

template<typename dtype>
CTF_Idx_Tensor<dtype>::CTF_Idx_Tensor(
    CTF_Idx_Tensor<dtype> const &  other,
    int                       copy,
    std::map<CTF_Tensor<dtype>*, CTF_Tensor<dtype>*>* remap) {
  if (other.parent == NULL){
    parent        = NULL;
    idx_map       = NULL;
    is_intm       = 0;
  } else {
    parent = other.parent;
    if (remap != NULL) {
      typename std::map<CTF_Tensor<dtype>*, CTF_Tensor<dtype>*>::iterator it = remap->find(parent);
      assert(it != remap->end()); // assume a remapping will be complete
      parent = it->second;
    }

    if (copy || other.is_intm){
      parent = new CTF_Tensor<dtype>(*parent,1);
      is_intm = 1;
    } else {
      // leave parent as is - already correct
      is_intm = 0;
    }
    idx_map = (char*)CTF_alloc(other.parent->ndim*sizeof(char));
    memcpy(idx_map, other.idx_map, parent->ndim*sizeof(char));
  }
  this->scale    = other.scale;
}

template<typename dtype>
CTF_Idx_Tensor<dtype>::CTF_Idx_Tensor(){
  parent        = NULL;
  idx_map       = NULL;
  is_intm       = 0;
  this->scale    = 1.0;
}

template<typename dtype>
CTF_Idx_Tensor<dtype>::CTF_Idx_Tensor(dtype val){
  parent        = NULL;
  idx_map       = NULL;
  is_intm       = 0;
  this->scale   = val;
}

template<typename dtype>
CTF_Idx_Tensor<dtype>::~CTF_Idx_Tensor(){
  if (is_intm) { 
    delete parent;
    is_intm = 0;
  }
  if (idx_map != NULL)  CTF_free(idx_map);
  idx_map = NULL;
}

template<typename dtype>
CTF_Term<dtype> * CTF_Idx_Tensor<dtype>::clone(std::map<CTF_Tensor<dtype>*, CTF_Tensor<dtype>*>* remap) const {
  return new CTF_Idx_Tensor<dtype>(*this, 0, remap);
}

template<typename dtype>
CTF_World<dtype> * CTF_Idx_Tensor<dtype>::where_am_i() const {
  if (parent == NULL) return NULL;
  return parent->world;
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator=(CTF_Idx_Tensor<dtype> const & B){
  if (global_schedule != NULL) {
    std::cout << "op= tensor" << std::endl;
    assert(false);
  } else {
    this->scale = 0.0;
    B.execute(*this);
    this->scale = 1.0;
  }
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator=(CTF_Term<dtype> const & B){
  if (global_schedule != NULL) {
    global_schedule->add_operation(
        new CTF_TensorOperation<dtype>(TENSOR_OP_SET, new CTF_Idx_Tensor(*this), B.clone()));
  } else {
    this->scale = 0.0;
    B.execute(*this);
    this->scale = 1.0;
  }
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator+=(CTF_Term<dtype> const & B){
  if (global_schedule != NULL) {
    global_schedule->add_operation(
        new CTF_TensorOperation<dtype>(TENSOR_OP_SUM, new CTF_Idx_Tensor(*this), B.clone()));
  } else {
    //this->scale = 1.0;
    B.execute(*this);
    this->scale = 1.0;
  }
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator-=(CTF_Term<dtype> const & B){
  if (global_schedule != NULL) {
    global_schedule->add_operation(
        new CTF_TensorOperation<dtype>(TENSOR_OP_SUBTRACT, new CTF_Idx_Tensor(*this), B.clone()));
  } else {
    CTF_Term<dtype> * Bcpy = B.clone();
    Bcpy->scale *= -1.0;
    Bcpy->execute(*this);
    this->scale = 1.0;
    delete Bcpy;
  }
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator*=(CTF_Term<dtype> const & B){
  if (global_schedule != NULL) {
    global_schedule->add_operation(
        new CTF_TensorOperation<dtype>(TENSOR_OP_MULTIPLY, new CTF_Idx_Tensor(*this), B.clone()));
  } else {
    CTF_Contract_Term<dtype> ctrm = (*this)*B;
    *this = ctrm;
  }
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::execute(CTF_Idx_Tensor<dtype> output) const {
  if (parent == NULL){
    output.scale *= this->scale;
    CTF_Scalar<dtype> ts(this->scale, *(output.where_am_i()));
    output.parent->sum(1.0, ts, "",
                       output.scale, output.idx_map);
  } else {
    output.parent->sum(this->scale, *this->parent, idx_map,
                       output.scale, output.idx_map);
  } 
}

template<typename dtype>
CTF_Idx_Tensor<dtype> CTF_Idx_Tensor<dtype>::execute() const {
  return *this;
}

template<typename dtype>
long_int CTF_Idx_Tensor<dtype>::estimate_cost(CTF_Idx_Tensor<dtype> output) const {
  long_int cost = 0;
  if (parent == NULL){
    CTF_Scalar<dtype> ts(this->scale, *(output.where_am_i()));
    cost += output.parent->estimate_cost(ts, "",
                       output.idx_map);
  } else {
    cost += output.parent->estimate_cost(*this->parent, idx_map,
                        output.idx_map);
  } 
  return cost;
}

template<typename dtype>
CTF_Idx_Tensor<dtype> CTF_Idx_Tensor<dtype>::estimate_cost(long_int & cost) const {
  return *this;
}

template<typename dtype>
void CTF_Idx_Tensor<dtype>::get_inputs(std::set<CTF_Tensor<dtype>*, tensor_tid_less<dtype> >* inputs_set) const {
  if (parent) {
    inputs_set->insert(parent);
  }
}

/*template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator=(dtype B){
  *this=(CTF_Scalar<dtype>(B,*(this->parent->world))[""]);
}
template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator+=(dtype B){
  *this+=(CTF_Scalar<dtype>(B,*(this->parent->world))[""]);
}
template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator-=(dtype B){
  *this-=(CTF_Scalar<dtype>(B,*(this->parent->world))[""]);
}
template<typename dtype>
void CTF_Idx_Tensor<dtype>::operator*=(dtype B){
  *this*=(CTF_Scalar<dtype>(B,*(this->parent->world))[""]);
}*/

/*
template<typename dtype>
CTF_Idx_Tensor<dtype> CTF_Idx_Tensor<dtype>::operator+(CTF_Idx_Tensor<dtype> tsr){
  if (has_contract || has_sum){
    *NBR = (*NBR)-tsr;
    return *this;
  }
  NBR = &tsr;
  has_sum = 1;
  return *this;
}

template<typename dtype>
CTF_Idx_Tensor<dtype> CTF_Idx_Tensor<dtype>::operator-(CTF_Idx_Tensor<dtype> tsr){
  if (has_contract || has_sum){
    *NBR = (*NBR)-tsr;
    return *this;
  }
  NBR = &tsr;
  has_sum = 1;
  if (tsr.has_scale) tsr.scale = -1.0*tsr.scale;
  else {
    tsr.has_scale = 1;
    tsr.scale = -1.0;
  }
  return *this;
}

template<typename dtype>
CTF_Idx_Tensor<dtype> CTF_Idx_Tensor<dtype>::operator*(double  scl){
  if (has_contract){
    *NBR =(*NBR)*scl;
    return *this;
  }
  if (has_scale){
    scale *= scl;
  } else {
    has_scale = 1;
    scale = scl;
  }
  return *this;
}*/

/*
template<typename dtype>
void CTF_Idx_Tensor<dtype>::run(CTF_Idx_Tensor<dtype>* output, dtype  beta){
  dtype  alpha;
  if (has_scale) alpha = scale;
  else alpha = 1.0;
  if (has_contract){
    if (NBR->has_scale){
      alpha *= NBR->scale;
    }
    if (NBR->has_contract || NBR->has_sum){
      CTF_Idx_Tensor itsr = get_intermediate(*this,*NBR);
      itsr.has_sum = NBR->has_sum;
      itsr.has_contract = NBR->has_contract;
      itsr.NBR = NBR->NBR;
      printf("interm tsr has_Contract = %d, NBR = %p, NBR.has_scale = %d\n", itsr.has_contract, itsr.NBR,
      itsr.NBR->has_scale);
      
      itsr.parent->contract(alpha, *(this->parent), this->idx_map,
                                    *(NBR->parent),  NBR->idx_map,
                             0.0,                    itsr.idx_map);
      itsr.run(output, beta);
    } else {
      output->parent->contract(alpha, *(this->parent), this->idx_map,
                                      *(NBR->parent),  NBR->idx_map,
                               beta,                   output->idx_map);
    }
  } else {
    if (has_sum){
      CTF_Tensor<dtype> tcpy(*(this->parent),1);
      CTF_Idx_Tensor itsr(&tcpy, idx_map);
      NBR->run(&itsr, alpha);
      output->parent->sum(1.0, tcpy, idx_map, beta, output->idx_map);
//      delete itsr;
//      delete tcpy;
    } else {
      output->parent->sum(alpha, *(this->parent), idx_map, beta, output->idx_map);
    }
  }  
//  if (!is_perm)
//    delete this;
}*/

