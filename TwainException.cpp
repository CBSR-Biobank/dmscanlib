#include "TwainException.h"

TwainException::TwainException(const char* msg) : pMessage(msg) 
{}
const char* TwainException::what() const
{
	return pMessage;
}
