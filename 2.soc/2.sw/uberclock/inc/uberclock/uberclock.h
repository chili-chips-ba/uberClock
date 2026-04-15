/**
 * @file uberclock.h
 * @brief Top-level firmware control interface.
 *
 * @defgroup core Core Runtime
 * @{
 */
#ifndef UBERCLOCK_H
#define UBERCLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/** Register all firmware commands */
void uberclock_register_cmds(void);

/** Initialize firmware runtime */
void uberclock_init(void);

/** Main polling loop hook */
void uberclock_poll(void);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
