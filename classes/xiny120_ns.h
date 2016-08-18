#ifndef __XINY120_NS_H
#define __XINY120_NS_H

#define NS_XINY120_BEGIN namespace xiny120 {
#define NS_XINY120_END };

#define NS_XINY120_NET_BEGIN namespace net {
#define NS_XINY120_NET_END };

#define X_VAR(varType, varName)\
protected: std::atomic<varType> m##varName;\
public: virtual const varType get##varName(void) const {return (m##varName);};\
public: virtual void set##varName(const varType& var){ (m##varName) = var;}; 

#define X_VAR_R(varType, varName)\
protected: std::atomic<varType> m##varName;\
public: virtual const varType get##varName(void) const {return (m##varName);};



#endif