/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   log_print.h
 * Author: gerard
 *
 * Created on December 15, 2020, 10:26 PM
 */

#ifndef LOG_PRINT_H
#define LOG_PRINT_H

#ifdef __cplusplus
extern "C" {
#endif

int log_open(const char *logfile);
void log_close(void);
void log_printf (char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* LOG_PRINT_H */

