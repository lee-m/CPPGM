#ifndef PPTOKEN_EXCEPTION_H
#define PPTOKEN_EXCEPTION_H

class pptoken_exception : public exception
{
private:
  string mMessage;

public:
  pptoken_exception(const string &message) : mMessage(message) {}
  ~pptoken_exception() throw() {}
  const char* what() const throw() { return mMessage.c_str(); }
};

#endif

