/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `win_log.cpp` for details.
 */
#pragma once

// Library of evaluation functions/macros that will check success of various result types
// (non-zero, "ERROR_SUCCESS" (i.e. check that result == 0), and HRESULT (check SUCCESS())).
// For each result type, if the result is not the success result, the function/macros will 
// throw an exception decended from class LoggingException. 
// LoggingException has a virtual  function "Log()" that can be called from the error handler (catch block). 
// This function will use the log.c functionality to log info based on the exception type/# to the log file setup 
// for the project. There is also an all - purpose function (generate_exception) that will throw a 
// LoggingException with a user-defined message.

// For details on log.c see https://github.com/rxi/log.c

extern "C"
{
#include "log.h"
}

extern const DWORD PROC_ID;

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

bool enable_logging(const std::string& szLogPath, SPFILE& spFIle);
bool enable_logging(const std::wstring& szLogPath, SPFILE& spFIle);

// Evaluate dwError for == ERROR_SUCCESS (i.e. == 0). if dwError !=ERROR_SUCCESS, throw a DWLoggingException. 
// DWLoggingException::Log() will call FormatMessage to provide the error description in the log ifle
DWORD eval_log_errorsuccess_getlasterror(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, DWORD dwError);

// Evaluate valNonZero for != 0. If valNonZero  == 0, throw a DWLoggingException. 
// DWLoggingException::Log() will call FormatMessage to provide the error description in the log ifle
template<class T>
T eval_log_nonzero_getlasterror(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, const T& valNonZero)
{
	if (valNonZero == 0) {
		throw DWLoggingException(level, file, line, function, functionCalled, GetLastError());
	}
	return valNonZero;
}

// Evaluate hr for SUCCESS(hr). If FAILED(hr), throw a HRLoggingException. 
// HRLoggingException::Log() will call GetErrorInfo to try to retrieve the the error description in the log file.
// If GetErrorInfo fails, falls back FormatMessage to provide the error description in the log file
HRESULT eval_log_getcomerror(int level, const char* file, int line, const std::string& function, const std::string& functionCalled, HRESULT hr);

//All-purpose function that will throw a MsgLoggingException with a user-defined message.
void generate_exception(int level, const char* file, int line, const std::string& function, const std::string& msg);

#define log_info_procid(fmt, ...) \
	log_info(("ProcID <%u>: " fmt), PROC_ID __VA_OPT__(,) __VA_ARGS__)

#define log_fatal_procid(fmt, ...) \
	log_fatal(("ProcID <%u>: " fmt), PROC_ID  __VA_OPT__(,) __VA_ARGS__)

// Macro definitions for each return type and Logging level. The macros will automatically provide
// the file, line, and function calling to the eval*/generate functions above.
// The eval* macros will also provide the function that was called
// The logging levels are defined in the log.c documentation. See https://github.com/rxi/log.c
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