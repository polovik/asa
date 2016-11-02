EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:main_board-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L LM324N U1
U 1 1 580E24D2
P 3700 2550
F 0 "U1" H 3750 2750 50  0000 C CNN
F 1 "LM324N" H 3850 2350 50  0000 C CNN
F 2 "Housings_DIP:DIP-14_W7.62mm_LongPads" H 3650 2650 50  0001 C CNN
F 3 "" H 3750 2750 50  0000 C CNN
	1    3700 2550
	1    0    0    -1  
$EndComp
$Comp
L LM324N U1
U 3 1 580E24D3
P 5000 2650
F 0 "U1" H 5050 2850 50  0000 C CNN
F 1 "LM324N" H 5150 2450 50  0000 C CNN
F 2 "Housings_DIP:DIP-14_W7.62mm_LongPads" H 4950 2750 50  0001 C CNN
F 3 "" H 5050 2850 50  0000 C CNN
	3    5000 2650
	1    0    0    -1  
$EndComp
$Comp
L LM324N U1
U 4 1 580E24D4
P 5000 3500
F 0 "U1" H 5050 3700 50  0000 C CNN
F 1 "LM324N" H 5150 3300 50  0000 C CNN
F 2 "Housings_DIP:DIP-14_W7.62mm_LongPads" H 4950 3600 50  0001 C CNN
F 3 "" H 5050 3700 50  0000 C CNN
	4    5000 3500
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR01
U 1 1 580E24D5
P 3200 4900
F 0 "#PWR01" H 3200 4650 50  0001 C CNN
F 1 "GND" H 3200 4750 50  0000 C CNN
F 2 "" H 3200 4900 50  0000 C CNN
F 3 "" H 3200 4900 50  0000 C CNN
	1    3200 4900
	1    0    0    -1  
$EndComp
$Comp
L LM324N U1
U 2 1 580E24D6
P 3700 4050
F 0 "U1" H 3750 4250 50  0000 C CNN
F 1 "LM324N" H 3850 3850 50  0000 C CNN
F 2 "Housings_DIP:DIP-14_W7.62mm_LongPads" H 3650 4150 50  0001 C CNN
F 3 "" H 3750 4250 50  0000 C CNN
	2    3700 4050
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 580E24D7
P 3100 2600
F 0 "R1" V 3180 2600 50  0000 C CNN
F 1 "1k" V 3100 2600 50  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.25W" V 3030 2600 50  0001 C CNN
F 3 "" H 3100 2600 50  0000 C CNN
	1    3100 2600
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 580E24D8
P 4300 2550
F 0 "R2" V 4380 2550 50  0000 C CNN
F 1 "1k" V 4300 2550 50  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.25W" V 4230 2550 50  0001 C CNN
F 3 "" H 4300 2550 50  0000 C CNN
	1    4300 2550
	0    1    1    0   
$EndComp
$Comp
L POT RV1
U 1 1 580E24D9
P 3400 3000
F 0 "RV1" H 3400 2920 50  0000 C CNN
F 1 "100k" H 3400 3000 50  0000 C CNN
F 2 "footprints:Potentiometer_CP3-36" H 3400 3000 50  0001 C CNN
F 3 "" H 3400 3000 50  0000 C CNN
	1    3400 3000
	1    0    0    -1  
$EndComp
$Comp
L POT RV3
U 1 1 580E24DA
P 5600 2800
F 0 "RV3" H 5600 2720 50  0000 C CNN
F 1 "100k" H 5600 2800 50  0000 C CNN
F 2 "footprints:Potentiometer_CP3-36" H 5600 2800 50  0001 C CNN
F 3 "" H 5600 2800 50  0000 C CNN
	1    5600 2800
	0    1    -1   0   
$EndComp
$Comp
L POT RV4
U 1 1 580E24DB
P 5600 3650
F 0 "RV4" H 5600 3570 50  0000 C CNN
F 1 "100k" H 5600 3650 50  0000 C CNN
F 2 "footprints:Potentiometer_CP3-36" H 5600 3650 50  0001 C CNN
F 3 "" H 5600 3650 50  0000 C CNN
	1    5600 3650
	0    1    -1   0   
$EndComp
$Comp
L GND #PWR02
U 1 1 580E24DC
P 5600 3000
F 0 "#PWR02" H 5600 2750 50  0001 C CNN
F 1 "GND" H 5600 2850 50  0000 C CNN
F 2 "" H 5600 3000 50  0000 C CNN
F 3 "" H 5600 3000 50  0000 C CNN
	1    5600 3000
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 580E24DD
P 5600 3850
F 0 "#PWR03" H 5600 3600 50  0001 C CNN
F 1 "GND" H 5600 3700 50  0000 C CNN
F 2 "" H 5600 3850 50  0000 C CNN
F 3 "" H 5600 3850 50  0000 C CNN
	1    5600 3850
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR04
U 1 1 580E24DE
P 3200 3100
F 0 "#PWR04" H 3200 2850 50  0001 C CNN
F 1 "GND" H 3200 2950 50  0000 C CNN
F 2 "" H 3200 3100 50  0000 C CNN
F 3 "" H 3200 3100 50  0000 C CNN
	1    3200 3100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR05
U 1 1 580E24DF
P 3100 2800
F 0 "#PWR05" H 3100 2550 50  0001 C CNN
F 1 "GND" H 3100 2650 50  0000 C CNN
F 2 "" H 3100 2800 50  0000 C CNN
F 3 "" H 3100 2800 50  0000 C CNN
	1    3100 2800
	1    0    0    -1  
$EndComp
NoConn ~ 3600 2850
NoConn ~ 3600 2250
NoConn ~ 4900 2350
NoConn ~ 4900 2950
NoConn ~ 4900 3200
NoConn ~ 4900 3800
$Comp
L +12V #PWR06
U 1 1 580E24E0
P 3600 3550
F 0 "#PWR06" H 3600 3400 50  0001 C CNN
F 1 "+12V" H 3600 3690 50  0000 C CNN
F 2 "" H 3600 3550 50  0000 C CNN
F 3 "" H 3600 3550 50  0000 C CNN
	1    3600 3550
	1    0    0    -1  
$EndComp
$Comp
L -12V #PWR7
U 1 1 580E24E1
P 3600 4550
F 0 "#PWR7" H 3600 4650 50  0001 C CNN
F 1 "-12V" H 3600 4700 50  0000 C CNN
F 2 "" H 3600 4550 50  0000 C CNN
F 3 "" H 3600 4550 50  0000 C CNN
	1    3600 4550
	-1   0    0    1   
$EndComp
$Comp
L CONN_01X01 P1
U 1 1 580E24EB
P 5050 1800
F 0 "P1" H 5050 1900 50  0000 C CNN
F 1 "Probe" V 5150 1800 50  0000 C CNN
F 2 "footprints:1pin_small" H 5050 1800 50  0001 C CNN
F 3 "" H 5050 1800 50  0000 C CNN
	1    5050 1800
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X01 P2
U 1 1 580E24EC
P 5050 2050
F 0 "P2" H 5050 2150 50  0000 C CNN
F 1 "GND" V 5150 2050 50  0000 C CNN
F 2 "footprints:1pin_small" H 5050 2050 50  0001 C CNN
F 3 "" H 5050 2050 50  0000 C CNN
	1    5050 2050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR07
U 1 1 580E24ED
P 4750 2100
F 0 "#PWR07" H 4750 1850 50  0001 C CNN
F 1 "GND" H 4750 1950 50  0000 C CNN
F 2 "" H 4750 2100 50  0000 C CNN
F 3 "" H 4750 2100 50  0000 C CNN
	1    4750 2100
	1    0    0    -1  
$EndComp
$Comp
L JACK_2P J1
U 1 1 580E24EE
P 2100 2600
F 0 "J1" H 1750 2400 50  0000 C CNN
F 1 "To Line OUT" H 1950 2850 50  0000 C CNN
F 2 "Connect:PINHEAD1-3" H 2100 2600 50  0001 C CNN
F 3 "" H 2100 2600 50  0000 C CNN
	1    2100 2600
	1    0    0    -1  
$EndComp
$Comp
L JACK_2P J2
U 1 1 580E24EF
P 7900 3250
F 0 "J2" H 7550 3050 50  0000 C CNN
F 1 "To Line IN" H 7750 3500 50  0000 C CNN
F 2 "Connect:PINHEAD1-3" H 7900 3250 50  0001 C CNN
F 3 "" H 7900 3250 50  0000 C CNN
	1    7900 3250
	-1   0    0    -1  
$EndComp
NoConn ~ 2550 2700
$Comp
L GND #PWR08
U 1 1 580E24F0
P 2750 2800
F 0 "#PWR08" H 2750 2550 50  0001 C CNN
F 1 "GND" H 2750 2650 50  0000 C CNN
F 2 "" H 2750 2800 50  0000 C CNN
F 3 "" H 2750 2800 50  0000 C CNN
	1    2750 2800
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR09
U 1 1 580E24F1
P 7050 3350
F 0 "#PWR09" H 7050 3100 50  0001 C CNN
F 1 "GND" H 7050 3200 50  0000 C CNN
F 2 "" H 7050 3350 50  0000 C CNN
F 3 "" H 7050 3350 50  0000 C CNN
	1    7050 3350
	1    0    0    -1  
$EndComp
$Comp
L D D1
U 1 1 580E24F2
P 4300 4400
F 0 "D1" H 4300 4500 50  0000 C CNN
F 1 "D226" H 4300 4300 50  0000 C CNN
F 2 "footprints:Diode_D226" H 4300 4400 50  0001 C CNN
F 3 "" H 4300 4400 50  0000 C CNN
	1    4300 4400
	-1   0    0    1   
$EndComp
$Comp
L R R3
U 1 1 580E24F3
P 4900 4550
F 0 "R3" V 4980 4550 50  0000 C CNN
F 1 "1k" V 4900 4550 50  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.25W" V 4830 4550 50  0001 C CNN
F 3 "" H 4900 4550 50  0000 C CNN
	1    4900 4550
	1    0    0    -1  
$EndComp
$Comp
L CP1 C3
U 1 1 580E24F4
P 4600 4550
F 0 "C3" H 4625 4650 50  0000 L CNN
F 1 "100u" H 4625 4450 50  0000 L CNN
F 2 "footprints:C100u_vert" H 4600 4550 50  0001 C CNN
F 3 "" H 4600 4550 50  0000 C CNN
	1    4600 4550
	1    0    0    -1  
$EndComp
$Comp
L LED D2
U 1 1 580E24F5
P 4900 5000
F 0 "D2" H 4900 5100 50  0000 C CNN
F 1 "LED" H 4900 4900 50  0000 C CNN
F 2 "footprints:Pin_vert_1x02" H 4900 5000 50  0001 C CNN
F 3 "" H 4900 5000 50  0000 C CNN
	1    4900 5000
	0    -1   -1   0   
$EndComp
$Comp
L POT RV2
U 1 1 580E24F6
P 3400 4800
F 0 "RV2" H 3400 4720 50  0000 C CNN
F 1 "100k" H 3400 4800 50  0000 C CNN
F 2 "footprints:Potentiometer_CP3-36" H 3400 4800 50  0001 C CNN
F 3 "" H 3400 4800 50  0000 C CNN
	1    3400 4800
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR010
U 1 1 580E24F7
P 4600 5250
F 0 "#PWR010" H 4600 5000 50  0001 C CNN
F 1 "GND" H 4600 5100 50  0000 C CNN
F 2 "" H 4600 5250 50  0000 C CNN
F 3 "" H 4600 5250 50  0000 C CNN
	1    4600 5250
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 580E24F8
P 5950 2800
F 0 "R4" V 6030 2800 50  0000 C CNN
F 1 "1k" V 5950 2800 50  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM15mm" V 5880 2800 50  0001 C CNN
F 3 "" H 5950 2800 50  0000 C CNN
	1    5950 2800
	0    1    1    0   
$EndComp
$Comp
L R R5
U 1 1 580E24F9
P 5950 3650
F 0 "R5" V 6030 3650 50  0000 C CNN
F 1 "1k" V 5950 3650 50  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM15mm" V 5880 3650 50  0001 C CNN
F 3 "" H 5950 3650 50  0000 C CNN
	1    5950 3650
	0    1    1    0   
$EndComp
$Comp
L D D5
U 1 1 580E24FA
P 6600 2950
F 0 "D5" H 6600 3050 50  0000 C CNN
F 1 "KD105" H 6600 2850 50  0000 C CNN
F 2 "footprints:Diode_KD105" H 6600 2950 50  0001 C CNN
F 3 "" H 6600 2950 50  0000 C CNN
	1    6600 2950
	0    1    1    0   
$EndComp
$Comp
L D D4
U 1 1 580E24FB
P 6250 3500
F 0 "D4" H 6250 3600 50  0000 C CNN
F 1 "KD105" H 6250 3400 50  0000 C CNN
F 2 "footprints:Diode_KD105" H 6250 3500 50  0001 C CNN
F 3 "" H 6250 3500 50  0000 C CNN
	1    6250 3500
	0    1    1    0   
$EndComp
$Comp
L D D3
U 1 1 580E24FC
P 6250 2950
F 0 "D3" H 6250 3050 50  0000 C CNN
F 1 "KD105" H 6250 2850 50  0000 C CNN
F 2 "footprints:Diode_KD105" H 6250 2950 50  0001 C CNN
F 3 "" H 6250 2950 50  0000 C CNN
	1    6250 2950
	0    -1   -1   0   
$EndComp
$Comp
L D D6
U 1 1 580E24FD
P 6600 3500
F 0 "D6" H 6600 3600 50  0000 C CNN
F 1 "KD105" H 6600 3400 50  0000 C CNN
F 2 "footprints:Diode_KD105" H 6600 3500 50  0001 C CNN
F 3 "" H 6600 3500 50  0000 C CNN
	1    6600 3500
	0    -1   -1   0   
$EndComp
$Comp
L C C1
U 1 1 580E24FF
P 3200 3650
F 0 "C1" H 3225 3750 50  0000 L CNN
F 1 "0.1u" H 3225 3550 50  0000 L CNN
F 2 "footprints:C_0.1u" H 3238 3500 50  0001 C CNN
F 3 "" H 3200 3650 50  0000 C CNN
	1    3200 3650
	0    1    1    0   
$EndComp
$Comp
L C C2
U 1 1 580E2500
P 3200 4450
F 0 "C2" H 3225 4550 50  0000 L CNN
F 1 "0.1u" H 3225 4350 50  0000 L CNN
F 2 "footprints:C_0.1u" H 3238 4300 50  0001 C CNN
F 3 "" H 3200 4450 50  0000 C CNN
	1    3200 4450
	0    1    1    0   
$EndComp
Wire Wire Line
	5300 2650 5600 2650
Wire Wire Line
	4700 2750 4700 3000
Wire Wire Line
	4700 3000 5350 3000
Wire Wire Line
	5350 3000 5350 2650
Connection ~ 5350 2650
Wire Wire Line
	5600 3500 5300 3500
Wire Wire Line
	4700 3600 4700 3850
Wire Wire Line
	4700 3850 5350 3850
Wire Wire Line
	5350 3850 5350 3500
Connection ~ 5350 3500
Wire Wire Line
	5600 3800 5600 3850
Wire Wire Line
	3400 2650 3400 2850
Wire Wire Line
	3550 3000 4050 3000
Wire Wire Line
	3200 3100 3200 3000
Wire Wire Line
	3200 3000 3250 3000
Wire Wire Line
	3100 2750 3100 2800
Wire Wire Line
	2550 2450 3400 2450
Wire Wire Line
	4700 2550 4450 2550
Wire Wire Line
	4000 2550 4150 2550
Wire Wire Line
	4050 2550 4050 3400
Connection ~ 4050 2550
Wire Wire Line
	4050 3400 4700 3400
Connection ~ 4050 3000
Wire Wire Line
	5600 3000 5600 2950
Wire Wire Line
	3600 4350 3600 4550
Wire Wire Line
	3600 3550 3600 3750
Wire Wire Line
	4850 2050 4750 2050
Wire Wire Line
	4750 2050 4750 2100
Wire Wire Line
	4600 1800 4600 2550
Connection ~ 4600 2550
Wire Wire Line
	4850 1800 4600 1800
Wire Wire Line
	2750 2800 2750 2600
Wire Wire Line
	2750 2600 2550 2600
Connection ~ 3100 2450
Wire Wire Line
	3400 4150 3400 4650
Wire Wire Line
	3200 4900 3200 4800
Wire Wire Line
	3200 4800 3250 4800
Wire Wire Line
	4050 4800 3550 4800
Wire Wire Line
	4050 4050 4050 4800
Wire Wire Line
	2950 2450 2950 3950
Connection ~ 2950 2450
Wire Wire Line
	4150 4400 4050 4400
Connection ~ 4050 4400
Wire Wire Line
	4450 4400 4900 4400
Connection ~ 4600 4400
Wire Wire Line
	4900 4700 4900 4800
Wire Wire Line
	4900 5200 4600 5200
Wire Wire Line
	4600 4700 4600 5250
Connection ~ 4600 5200
Wire Wire Line
	5800 3650 5750 3650
Wire Wire Line
	5800 2800 5750 2800
Wire Wire Line
	6100 2800 7450 2800
Connection ~ 6250 2800
Connection ~ 6600 2800
Wire Wire Line
	6100 3650 7450 3650
Connection ~ 6600 3650
Connection ~ 6250 3650
Wire Wire Line
	6250 3100 6250 3350
Wire Wire Line
	7450 2800 7450 3100
Wire Wire Line
	7450 3650 7450 3350
Wire Wire Line
	6250 3250 7450 3250
Wire Wire Line
	4050 4050 4000 4050
Wire Wire Line
	3350 3650 3600 3650
Connection ~ 3600 3650
Wire Wire Line
	3050 3650 3050 4450
Wire Wire Line
	3600 4450 3350 4450
Connection ~ 3600 4450
Wire Wire Line
	2950 3950 3400 3950
$Comp
L GND #PWR011
U 1 1 580E2501
P 2900 4200
F 0 "#PWR011" H 2900 3950 50  0001 C CNN
F 1 "GND" H 2900 4050 50  0000 C CNN
F 2 "" H 2900 4200 50  0000 C CNN
F 3 "" H 2900 4200 50  0000 C CNN
	1    2900 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3050 4150 2900 4150
Wire Wire Line
	2900 4150 2900 4200
Connection ~ 3050 4150
Connection ~ 6250 3250
Connection ~ 6600 3250
Wire Wire Line
	7050 3350 7050 3250
Connection ~ 7050 3250
Wire Wire Line
	6600 3100 6600 3350
Text Notes 6200 4150 0    60   ~ 0
WARNING:\nDo not connect J2 to Line IN during adjustment\n
$EndSCHEMATC