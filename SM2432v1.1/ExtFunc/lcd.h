/*
 * lcd.h
 *
 *  Created on: 12 Feb 2013
 *      Author: Ant
 */

#ifndef LCD_H_
#define LCD_H_

void PulseLcm();
void SendByte(char ByteToSend, int IsData);
void ClearLcmScreen();
void InitializeLcm(void);
void LcmSetCursorPosition(char Row, char Col);
void PrintStr(char *Text);


#endif /* LCD_H_ */
