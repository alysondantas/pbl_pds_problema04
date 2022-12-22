/**********************************************************************
* © 2007 Microchip Technology Inc.
*
* FileName:        main.c
* Dependencies:    Header (.h) files if applicable, see below
* Processor:       dsPIC33FJ256GP506
* Compiler:        MPLAB® C30 v3.00 or higher
*
*SOFTWARE LICENSE AGREEMENT:
*Except as otherwise expressly provided below, Microchip Technology Inc. 
*(Microchip) licenses this software to you solely for use with Microchip products.
*This software is owned by Microchip and/or its licensors, and is protected under 
*applicable copyright laws.  All rights reserved.
*
*This software and any accompanying information is for suggestion only.  
*It shall not be deemed to modify Microchip’s standard warranty for its products.
*It is your responsibility to ensure that this software meets your requirements.
*
*WARRANTY DISCLAIMER AND LIMIT ON LIABILITY:  SOFTWARE IS PROVIDED AS IS.  
*MICROCHIP AND ITS LICENSORS EXPRESSLY DISCLAIM ANY WARRANTY OF ANY KIND, 
*WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
*OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. 
*IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE FOR ANY INCIDENTAL, 
*SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, 
*HARM TO YOUR EQUIPMENT, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY 
*OR SERVICES, ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY 
*DEFENSE THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER SIMILAR 
*COSTS.  To the fullest extend allowed by law, Microchip and its licensors 
*liability shall not exceed the amount of fees, if any, that you have paid 
*directly to Microchip to use this software.
*
*THIRD PARTY SOFTWARE:  Notwithstanding anything to the contrary, any third 
*party software accompanying this software including but not limited to ITU 
*software is subject to the terms and conditions of the third party’s license
*agreement such as the General Public License.  To the extent required by third
*party licenses covering such third party software, the terms of such license 
*will apply in lieu of the terms provided herein.  To the extent the terms of 
*such third party licenses prohibit any of the restrictions described herein, 
*such restrictions will not apply to such third party software.  THIRD PARTY 
*SOFTWARE  IS SUBJECT TO THE FOREGOING WARRANTY DISCLAIMER AND LIMIT ON LIABILITY 
*PROVIDED IN THE PARAGRAPH ABOVE
*
*MICROCHIP PROVIDES THIS SOFTWARE (INCLUDING ACCOMPANYING THIRD PARTY SOFTWARE)
*CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE TERMS.
************************************************************************/

#include <p33FJ256GP506.h>
#include "..\h\OCPWMDrv.h"
//#include "..\h\sask.h"
#include "..\h\SFMDrv.h"
//#include "..\h\G711.h"
#include "..\h\ADCChannelDrv.h"
#include <dsp.h>
#include <libpic30.h>

_FGS(GWRP_OFF & GCP_OFF);
_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_ON & POSCMD_NONE);
_FWDT(FWDTEN_OFF);

/* FRAME_SIZE - Size of each audio frame 	
 * SPEECH_SEGMENT_SIZE - Size of intro speech segment
 * WRITE_START_ADDRESS - Serial Flash Memory write address					
 * */

#define FRAME_SIZE 				128
#define SPEECH_SEGMENT_SIZE		98049L		
#define WRITE_START_ADDRESS	0x20000		
#define FFT_BLOCK_LENGTH  		128 
#define N 88


/* Allocate memory for buffers and drivers	*/

int		adcBuffer		[ADC_CHANNEL_DMA_BUFSIZE] 	__attribute__((space(dma)));
int		ocPWMBuffer		[OCPWM_DMA_BUFSIZE]		__attribute__((space(dma)));
int		samples			[FRAME_SIZE];
int     samplesOut      [FRAME_SIZE];
char 	encodedSamples	[FRAME_SIZE];
int 	decodedSamples	[FRAME_SIZE];
//float   h[FRAME_SIZE];
//int   y[FRAME_SIZE];
char 	flashMemoryBuffer	[SFMDRV_BUFFER_SIZE];

//float y[FRAME_SIZE];

//fractional y[FFT_BLOCK_LENGTH] __attribute__((space(ymemory),  aligned(FFT_BLOCK_LENGTH*2))); 

//float samples_frac[FFT_BLOCK_LENGTH];

//fractional samples_frac[FFT_BLOCK_LENGTH] __attribute__((space(ymemory),  aligned(FFT_BLOCK_LENGTH*2))); 

fractional window[FFT_BLOCK_LENGTH] __attribute__((space(ymemory),  aligned(FFT_BLOCK_LENGTH*2))); 

fractcomplex twiddleFactorsFFT[FFT_BLOCK_LENGTH/2] 	__attribute__ ((section (".xbss, bss, xmemory"), aligned (FFT_BLOCK_LENGTH*2)));

fractcomplex twiddleFactorsIFFT[FFT_BLOCK_LENGTH/2] __attribute__ ((section (".xbss, bss, xmemory"), aligned (FFT_BLOCK_LENGTH*2)));

//__attribute__((space(xmemory),aligned(256))) fractional h[FRAME_SIZE];

int h[FRAME_SIZE];
int samplesBuffer[N];

/* Instantiate the drivers 	*/
ADCChannelHandle adcChannelHandle;
OCPWMHandle 	ocPWMHandle;

/* Create the driver handles	*/
ADCChannelHandle *pADCChannelHandle 	= &adcChannelHandle;
OCPWMHandle 	*pOCPWMHandle 		= &ocPWMHandle;

/* Addresses 
 * currentReadAddress - This one tracks the intro message	
 * currentWriteAddress - This one tracks the writes to flash	
 * userPlaybackAddress - This one tracks user playback		
 * address - Used during flash erase
 * */

 long currentReadAddress;		
 long currentWriteAddress;		
 long userPlaybackAddress;
 long address;	
 
 /* flags
 * record - if set means recording
 * playback - if set mean playback
 * erasedBeforeRecord - means SFM eras complete before record
 * */	

int record;						
int playback;						
int erasedBeforeRecord;

void FIR_InitConfig()
{
        /*h[0] = -4; //Para fc = 350 e Bw = 600
        h[1] = -8;
        h[2] = -13;
        h[3] = -20;
        h[4] = -28;
        h[5] = -37;
        h[6] = -45;
        h[7] = -49;
        h[8] = -46;
        h[9] = -31;
        h[10] = -3;
        h[11] = 43;
        h[12] = 106;
        h[13] = 186;
        h[14] = 281;
        h[15] = 386;
        h[16] = 496;
        h[17] = 604;
        h[18] = 702;
        h[19] = 783;
        h[20] = 841;
        h[21] = 871;
        h[22] = 871;
        h[23] = 841;
        h[24] = 783;
        h[25] = 702;
        h[26] = 604;
        h[27] = 496;
        h[28] = 386;
        h[29] = 281;
        h[30] = 186;
        h[31] = 106;
        h[32] = 43;
        h[33] = -3;
        h[34] = -31;
        h[35] = -46;
        h[36] = -49;
        h[37] = -45;
        h[38] = -37;
        h[39] = -28;
        h[40] = -20;
        h[41] = -13;
        h[42] = -8;
        h[43] = -4;*/
    
        /*h[0] = 0; //fc = 2000 e Bw = 500
        h[1] = 11;
        h[2] = 0;
        h[3] = -15;
        h[4] = 0;
        h[5] = 24;
        h[6] = 0;
        h[7] = -39;
        h[8] = 0;
        h[9] = 61;
        h[10] = 0;
        h[11] = -91;
        h[12] = 0;
        h[13] = 132;
        h[14] = 0;
        h[15] = -188;
        h[16] = 0;
        h[17] = 267;
        h[18] = 0;
        h[19] = -384;
        h[20] = 0;
        h[21] = 585;
        h[22] = 0;
        h[23] = -1029;
        h[24] = 0;
        h[25] = 3172;
        h[26] = 5000;
        h[27] = 3172;
        h[28] = 0;
        h[29] = -1029;
        h[30] = 0;
        h[31] = 585;
        h[32] = 0;
        h[33] = -384;
        h[34] = 0;
        h[35] = 267;
        h[36] = 0;
        h[37] = -188;
        h[38] = 0;
        h[39] = 132;
        h[40] = 0;
        h[41] = -91;
        h[42] = 0;
        h[43] = 61;
        h[44] = 0;
        h[45] = -39;
        h[46] = 0;
        h[47] = 24;
        h[48] = 0;
        h[49] = -15;
        h[50] = 0;
        h[51] = 11;
        h[52] = 0;*/
    
    /*h[0] = 6; // fc = 50 e Bw = 300
    h[1] = 6;
    h[2] = 6;
    h[3] = 7;
    h[4] = 8;
    h[5] = 9;
    h[6] = 10;
    h[7] = 12;
    h[8] = 14;
    h[9] = 16;
    h[10] = 18;
    h[11] = 20;
    h[12] = 23;
    h[13] = 26;
    h[14] = 29;
    h[15] = 33;
    h[16] = 36;
    h[17] = 40;
    h[18] = 44;
    h[19] = 48;
    h[20] = 52;
    h[21] = 56;
    h[22] = 61;
    h[23] = 65;
    h[24] = 70;
    h[25] = 74;
    h[26] = 78;
    h[27] = 83;
    h[28] = 87;
    h[29] = 91;
    h[30] = 95;
    h[31] = 99;
    h[32] = 103;
    h[33] = 106;
    h[34] = 109;
    h[35] = 112;
    h[36] = 115;
    h[37] = 117;
    h[38] = 120;
    h[39] = 121;
    h[40] = 123;
    h[41] = 124;
    h[42] = 125;
    h[43] = 125;
    h[44] = 125;
    h[45] = 125;
    h[46] = 124;
    h[47] = 123;
    h[48] = 121;
    h[49] = 120;
    h[50] = 117;
    h[51] = 115;
    h[52] = 112;
    h[53] = 109;
    h[54] = 106;
    h[55] = 103;
    h[56] = 99;
    h[57] = 95;
    h[58] = 91;
    h[59] = 87;
    h[60] = 83;
    h[61] = 78;
    h[62] = 74;
    h[63] = 70;
    h[64] = 65;
    h[65] = 61;
    h[66] = 56;
    h[67] = 52;
    h[68] = 48;
    h[69] = 44;
    h[70] = 40;
    h[71] = 36;
    h[72] = 33;
    h[73] = 29;
    h[74] = 26;
    h[75] = 23;
    h[76] = 20;
    h[77] = 18;
    h[78] = 16;
    h[79] = 14;
    h[80] = 12;
    h[81] = 10;
    h[82] = 9;
    h[83] = 8;
    h[84] = 7;
    h[85] = 6;
    h[86] = 6;
    h[87] = 6;*/
    
    
 h[0] = 0;
h[1] = 0;
h[2] = 0;
h[3] = 0;
h[4] = 0;
h[5] = 2;
h[6] = 0;
h[7] = 3;
h[8] = -4;
h[9] = 0;
h[10] = -10;
h[11] = 4;
h[12] = -5;
h[13] = 20;
h[14] = -1;
h[15] = 22;
h[16] = -26;
h[17] = 2;
h[18] = -52;
h[19] = 20;
h[20] = -23;
h[21] = 85;
h[22] = -4;
h[23] = 83;
h[24] = -96;
h[25] = 6;
h[26] = -177;
h[27] = 65;
h[28] = -75;
h[29] = 266;
h[30] = -12;
h[31] = 250;
h[32] = -287;
h[33] = 18;
h[34] = -530;
h[35] = 198;
h[36] = -232;
h[37] = 860;
h[38] = -41;
h[39] = 936;
h[40] = -1225;
h[41] = 96;
h[42] = -4063;
h[43] = 3928;
h[44] = 3928;
h[45] = -4063;
h[46] = 96;
h[47] = -1225;
h[48] = 936;
h[49] = -41;
h[50] = 860;
h[51] = -232;
h[52] = 198;
h[53] = -530;
h[54] = 18;
h[55] = -287;
h[56] = 250;
h[57] = -12;
h[58] = 266;
h[59] = -75;
h[60] = 65;
h[61] = -177;
h[62] = 6;
h[63] = -96;
h[64] = 83;
h[65] = -4;
h[66] = 85;
h[67] = -23;
h[68] = 20;
h[69] = -52;
h[70] = 2;
h[71] = -26;
h[72] = 22;
h[73] = -1;
h[74] = 20;
h[75] = -5;
h[76] = 4;
h[77] = -10;
h[78] = 0;
h[79] = -4;
h[80] = 3;
h[81] = 0;
h[82] = 2;
h[83] = 0;
h[84] = 0;
h[85] = 0;
h[86] = 0;
h[87] = 0;
}

//convoluir2(int samplesResource[],int xSize, float sistema[], int hSize, int samplesOutputConv[])
//convoluir2(samples, FRAME_SIZE, h,N, samplesOutputConv);

void convolution(int samplesOutputConv[]) {
    //int carryBuffer[N];
    int i =0;
    /*for(i = 0; i<5; i++) {
        carryBuffer[i] = 0;
    }*/
    long int y_aux = 0, hn =0;
    int  j, currentSample, amostra, k;
    for (i = 0; i < FRAME_SIZE + N -1; i++) {
        for (j = 0; j <N; j++) {
            if (i-j <0) {
                continue;
            }
            if (i - j < N-1) {
                k = i - j;
                currentSample = samplesBuffer[k];
            } else {
                k = i - j - N +1;
                currentSample = samples[k];
            }
            hn = h[j];
            hn = hn * currentSample;
            y_aux += hn;
        }
        
        if( i > N-2 ){
            amostra = (int) (y_aux/1000); // Desnormalizando e atribuindo ganho de 10x.
            samplesOutputConv[i-N+1] = amostra;
        }
        y_aux = 0;
    }
    
    for(i = 0; i<N-1; i++){
        samplesBuffer[i] = samples[FRAME_SIZE - N +1 +i];
    }
}

int main(void)
{
    FIR_InitConfig();
    
    //BlackmanInit( FFT_BLOCK_LENGTH, &window[128]); 
    
	/* Addresses 
	 * currentReadAddress - This one tracks the intro message	
	 * currentWriteAddress - This one tracks the writes to flash	
	 * userPlaybackAddress - This one tracks user playback		
	 * address - Used during flash erase
	 * */

	long currentReadAddress = 0;		
	long currentWriteAddress = WRITE_START_ADDRESS;		
	long userPlaybackAddress = WRITE_START_ADDRESS;		
	long address = 0;							

	/* flags
	 * record - if set means recording
	 * playback - if set mean playback
	 * erasedBeforeRecord - means SFM eras complete before record
	 * */

	//int record = 0;						
	//int playback = 0;					
	//int erasedBeforeRecord = 0;			

	/* Configure Oscillator to operate the device at 40MHz.
	 * Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
	 * Fosc= 7.37M*40/(2*2)=80Mhz for 7.37M input clock */
	 
	PLLFBD=41;				/* M=39	*/
	CLKDIVbits.PLLPOST=0;		/* N1=2	*/
	CLKDIVbits.PLLPRE=0;		/* N2=2	*/
	OSCTUN=0;			
	
	__builtin_write_OSCCONH(0x01);		/*	Initiate Clock Switch to FRC with PLL*/
	__builtin_write_OSCCONL(0x01);
	while (OSCCONbits.COSC != 0b01);	/*	Wait for Clock switch to occur	*/
	while(!OSCCONbits.LOCK);

	
	/* Intialize the board and the drivers	*/
	//SASKInit();
	ADCChannelInit	(pADCChannelHandle,adcBuffer);			/* For the ADC	*/
	OCPWMInit		(pOCPWMHandle,ocPWMBuffer);			/* For the OCPWM	*/

	/* Open the flash and unprotect it so that
	 * it can be written to.
	 * */

	SFMInit(flashMemoryBuffer);
	
		
	/* Start Audio input and output function	*/
	ADCChannelStart	(pADCChannelHandle);
	OCPWMStart		(pOCPWMHandle);	
		
	
	/* Main processing loop. Executed for every input and 
	 * output frame	*/
    
    //int tamanho = (2*N-1);
    //float y[tamanho];
    
    int i =0;
    long int y_aux = 0, hn =0;
    int  j, currentSample, amostra, k;
	 
	while(1)
	{
			/* Obtaing the ADC samples	*/
			while(ADCChannelIsBusy(pADCChannelHandle));
			ADCChannelRead	(pADCChannelHandle,samples,FRAME_SIZE);
			
            /*for(i=0; i<FRAME_SIZE; i++)
            {
                samplesOut[i] = (samples[i] * (3.3/4095)); //4096 = 12bits e 3.3 é vref normalização
            }*/
            
            
            //convolution(samplesOut);
            
            
            y_aux = 0;
            hn =0;
            j = 0;
            currentSample = 0;
            amostra = 0;
            k = 0;
            
            for(i = 0; i<5; i++) {
                samplesBuffer[i] = 0;
            }
            
            for (i = 0; i < FRAME_SIZE + N -1; i++) {
                for (j = 0; j <N; j++) {
                    if (i-j <0) {
                        continue;
                    }
                    if (i - j < N-1) {
                        k = i - j;
                        currentSample = samplesBuffer[k]; // obtem a amostra atual para multiplicar atraves das amostras do buffer
                    } else {
                        k = i - j - N +1;
                        currentSample = samples[k]; // obtem a amostra atual para multiplicar
                    }
                    hn = h[j];
                    hn = hn * currentSample; //multiplicação do h[n] por x[i - n + 1]
                    y_aux += hn;
                }

                if( i > N-2 ){
                    amostra = (int) (y_aux/1000); // Desnormalizando e atribuindo ganho de 10x.
                    samplesOut[i-N+1] = amostra;
                }
                y_aux = 0;
            }

            for(i = 0; i<N-1; i++){
                samplesBuffer[i] = samples[FRAME_SIZE - N +1 +i]; //atribui ao buffer amostras
            }
	
			/* Wait till the OC is available for a new  frame	*/
			while(OCPWMIsBusy(pOCPWMHandle));	
		
			/* Write the frame to the output	*/
			OCPWMWrite (pOCPWMHandle,samplesOut,FRAME_SIZE);
			
			/* The CheckSwitch functions are defined in sask.c	*/
			
	
	}
	
	
}
