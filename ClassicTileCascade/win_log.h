/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `win_log.cpp` for details.
 */
#pragma once

extern "C"
{
#include "log.h"
}

class LoggingException 
{
protected:
	int m_level;
	std::string m_file;
	int m_line;	
	std::string m_function;
	std::string m_functionCalled;

public:

	LoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled);

	virtual void Log() const = 0;

protected:
	static bool FormatMsg(DWORD dwErrVal, std::string& szErrMsg);
};

class HRLoggingException : public LoggingException
{
protected:
	HRESULT m_errVal;
public:

	HRLoggingException() = delete;
	HRLoggingException(const HRLoggingException&) = default;
	HRLoggingException(HRLoggingException&&) noexcept = default;
	HRLoggingException& operator=(const HRLoggingException&) = default;
	HRLoggingException& operator=(HRLoggingException&&) noexcept = default;

	HRLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, HRESULT errVal);

	void Log() const override;
};

class DWLoggingException : public LoggingException
{
protected:
	DWORD m_errVal;
public:
	DWLoggingException() = delete;
	DWLoggingException(const DWLoggingException&) = default;
	DWLoggingException(DWLoggingException&&) noexcept = default;
	DWLoggingException& operator=(const DWLoggingException&) = default;
	DWLoggingException& operator=(DWLoggingException&&) noexcept = default;

	DWLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, DWORD errVal);

	void Log() const override;
};

class MSGLoggingException : public LoggingException
{
protected:
	std::string m_errMsg;
public:
	MSGLoggingException() = delete;
	MSGLoggingException(const MSGLoggingException&) = default;
	MSGLoggingException(MSGLoggingException&&) noexcept = default;
	MSGLoggingException& operator=(const MSGLoggingException&) = default;
	MSGLoggingException& operator=(MSGLoggingException&&) noexcept = default;


	MSGLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& errMsg);

	void Log() const override;
};

DWORD eval_log_errorsuccess_getlasterror(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, DWORD dwError);

template<class T>
T eval_log_nonzero_getlasterror(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, const T& valNonZero)
{
	if (valNonZero == 0) {
		throw DWLoggingException(level, file, line, function, functionCalled, GetLastError());
	}
	return valNonZero;
}

HRESULT eval_log_getcomerror(int level, const char* file, int line, const std::string& function, const std::string& functionCalled, HRESULT hr);

void generate_exception(int level, const char* file, int line, const std::string& function, const std::string& msg);

#define eval_trace_es(fn) eval_log_errorsuccess_getlasterror(LOG_TRACE, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_debug_es(fn) eval_log_errorsuccess_getlasterror(LOG_DEBUG, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_info_es(fn) eval_log_errorsuccess_getlasterror(LOG_INFO, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_warn_es(fn) eval_log_errorsuccess_getlasterror(LOG_WARN, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_error_es(fn) eval_log_errorsuccess_getlasterror(LOG_ERROR, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_fatal_es(fn) eval_log_errorsuccess_getlasterror(LOG_FATAL, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)

#define eval_trace_nz(fn) eval_log_nonzero_getlasterror(LOG_TRACE, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_debug_nz(fn) eval_log_nonzero_getlasterror(LOG_DEBUG, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_info_nz(fn) eval_log_nonzero_getlasterror(LOG_INFO, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_warn_nz(fn) eval_log_nonzero_getlasterror(LOG_WARN, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_error_nz(fn) eval_log_nonzero_getlasterror(LOG_ERROR, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_fatal_nz(fn) eval_log_nonzero_getlasterror(LOG_FATAL, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)

#define eval_trace_hr(fn) eval_log_getcomerror(LOG_TRACE, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_debug_hr(fn) eval_log_getcomerror(LOG_DEBUG, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_info_hr(fn) eval_log_getcomerror(LOG_INFO, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_warn_hr(fn) eval_log_getcomerror(LOG_WARN, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_error_hr(fn) eval_log_getcomerror(LOG_ERROR, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)
#define eval_fatal_hr(fn) eval_log_getcomerror(LOG_FATAL, __FILE__, __LINE__, __FUNCSIG__, #fn,  fn)

#define generate_error(msg) generate_exception(LOG_ERROR, __FILE__, __LINE__, __FUNCSIG__, msg)
#define generate_fatal(msg) generate_exception(LOG_FATAL, __FILE__, __LINE__, __FUNCSIG__, msg)