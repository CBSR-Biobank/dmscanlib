#include "stdafx.h"
#include <iostream>
#include <exception>

class TwainException {

public:
   TwainException(const char* msg = "Error in Image Grabber");
   const char* what() const;
private:
   const char* pMessage;
};