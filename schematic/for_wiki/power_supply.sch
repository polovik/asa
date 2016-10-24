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
LIBS:asa-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date "6 aug 2016"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L CP1-RESCUE-asa C3
U 1 1 580FEC8C
P 4800 4100
F 0 "C3" H 4850 4200 50  0000 L CNN
F 1 "100u" H 4850 4000 50  0000 L CNN
F 2 "footprints:C100u_horiz" H 4800 4100 60  0001 C CNN
F 3 "~" H 4800 4100 60  0000 C CNN
	1    4800 4100
	1    0    0    -1  
$EndComp
$Comp
L CP1-RESCUE-asa C2
U 1 1 580FEC8D
P 3900 4900
F 0 "C2" H 3950 5000 50  0000 L CNN
F 1 "1000u" H 3950 4800 50  0000 L CNN
F 2 "footprints:C_Radial_D12.5_L25_P5_1000u" H 3500 6150 60  0001 C CNN
F 3 "~" H 3900 4900 60  0000 C CNN
	1    3900 4900
	1    0    0    1   
$EndComp
$Comp
L CP1-RESCUE-asa C4
U 1 1 580FEC8E
P 4800 5500
F 0 "C4" H 4850 5600 50  0000 L CNN
F 1 "100u" H 4850 5400 50  0000 L CNN
F 2 "footprints:C100u_horiz" H 4800 5500 60  0001 C CNN
F 3 "~" H 4800 5500 60  0000 C CNN
	1    4800 5500
	1    0    0    1   
$EndComp
$Comp
L CP1-RESCUE-asa C1
U 1 1 580FEC8F
P 3900 3500
F 0 "C1" H 3950 3600 50  0000 L CNN
F 1 "1000u" H 3950 3400 50  0000 L CNN
F 2 "footprints:C_Radial_D12.5_L25_P5_1000u" H 3900 4250 60  0001 C CNN
F 3 "~" H 3900 3500 60  0000 C CNN
	1    3900 3500
	1    0    0    -1  
$EndComp
$Comp
L CP1-RESCUE-asa C5
U 1 1 580FEC90
P 5300 3500
F 0 "C5" H 5350 3600 50  0000 L CNN
F 1 "470u" H 5350 3400 50  0000 L CNN
F 2 "footprints:C_Radial_D13_L21_P5_470u" H 5300 3500 60  0001 C CNN
F 3 "~" H 5300 3500 60  0000 C CNN
	1    5300 3500
	1    0    0    -1  
$EndComp
$Comp
L CP1-RESCUE-asa C6
U 1 1 580FEC91
P 5300 4900
F 0 "C6" H 5350 5000 50  0000 L CNN
F 1 "470u" H 5350 4800 50  0000 L CNN
F 2 "footprints:C_Radial_D13_L21_P5_470u" H 5300 4900 60  0001 C CNN
F 3 "~" H 5300 4900 60  0000 C CNN
	1    5300 4900
	1    0    0    1   
$EndComp
$Comp
L NPN Q1
U 1 1 580FEC92
P 4800 3400
F 0 "Q1" H 4800 3250 50  0000 R CNN
F 1 "KT815" H 4800 3550 50  0000 R CNN
F 2 "footprints:SOT126_SOT32_Housing_Vertical_KT81x" H 4800 3400 60  0001 C CNN
F 3 "~" H 4800 3400 60  0000 C CNN
	1    4800 3400
	0    -1   -1   0   
$EndComp
$Comp
L PNP Q2
U 1 1 580FEC93
P 4800 4800
F 0 "Q2" H 4800 4650 60  0000 R CNN
F 1 "KT814" H 4800 4950 60  0000 R CNN
F 2 "footprints:SOT126_SOT32_Housing_Vertical_KT81x" H 4800 4800 60  0001 C CNN
F 3 "~" H 4800 4800 60  0000 C CNN
	1    4800 4800
	0    -1   -1   0   
$EndComp
$Comp
L R-RESCUE-asa R1
U 1 1 580FEC94
P 4300 3550
F 0 "R1" V 4380 3550 40  0000 C CNN
F 1 "470" V 4307 3551 40  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.5W" V 4230 3550 30  0001 C CNN
F 3 "~" H 4300 3550 30  0000 C CNN
	1    4300 3550
	1    0    0    -1  
$EndComp
$Comp
L R-RESCUE-asa R2
U 1 1 580FEC95
P 4300 4950
F 0 "R2" V 4380 4950 40  0000 C CNN
F 1 "470" V 4307 4951 40  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.5W" V 4230 4950 30  0001 C CNN
F 3 "~" H 4300 4950 30  0000 C CNN
	1    4300 4950
	1    0    0    -1  
$EndComp
$Comp
L R-RESCUE-asa R4
U 1 1 580FEC96
P 5700 4950
F 0 "R4" V 5780 4950 40  0000 C CNN
F 1 "1200" V 5707 4951 40  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.25W" V 5630 4950 30  0001 C CNN
F 3 "~" H 5700 4950 30  0000 C CNN
	1    5700 4950
	1    0    0    -1  
$EndComp
$Comp
L R-RESCUE-asa R3
U 1 1 580FEC97
P 5700 3550
F 0 "R3" V 5780 3550 40  0000 C CNN
F 1 "1200" V 5707 3551 40  0000 C CNN
F 2 "footprints:Resistor_Horizontal_RM20mm_0.25W" V 5630 3550 30  0001 C CNN
F 3 "~" H 5700 3550 30  0000 C CNN
	1    5700 3550
	1    0    0    -1  
$EndComp
$Comp
L DIODESCH D2
U 1 1 580FEC98
P 4300 4100
F 0 "D2" H 4300 4200 40  0000 C CNN
F 1 "D814D" H 4300 4000 40  0000 C CNN
F 2 "footprints:Diode_P600_Horizontal_D814" H 4300 4100 60  0001 C CNN
F 3 "~" H 4300 4100 60  0000 C CNN
	1    4300 4100
	0    1    -1   0   
$EndComp
$Comp
L DIODESCH D3
U 1 1 580FEC99
P 4300 5500
F 0 "D3" H 4300 5600 40  0000 C CNN
F 1 "D814D" H 4300 5400 40  0000 C CNN
F 2 "footprints:Diode_P600_Horizontal_D814" H 4300 5500 60  0001 C CNN
F 3 "~" H 4300 5500 60  0000 C CNN
	1    4300 5500
	0    1    1    0   
$EndComp
$Comp
L Diode_Bridge D1
U 1 1 580FEC9A
P 2900 3600
F 0 "D1" H 2650 3900 50  0000 C CNN
F 1 "KC405" H 2900 3600 50  0000 C CNN
F 2 "footprints:Diode_Bridge_22x22" H 2900 3600 50  0001 C CNN
F 3 "" H 2900 3600 50  0000 C CNN
	1    2900 3600
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X01 P3
U 1 1 580FEC9B
P 6200 3300
F 0 "P3" H 6200 3400 50  0000 C CNN
F 1 "+12V" V 6300 3300 50  0000 C CNN
F 2 "footprints:1pin_big" H 6200 3300 50  0001 C CNN
F 3 "" H 6200 3300 50  0000 C CNN
	1    6200 3300
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X01 P1
U 1 1 580FECA0
P 900 4100
F 0 "P1" H 900 4200 50  0000 C CNN
F 1 "CONN_01X01" V 1000 4100 50  0001 C CNN
F 2 "footprints:1pin_big" H 900 4100 50  0001 C CNN
F 3 "" H 900 4100 50  0000 C CNN
	1    900  4100
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X01 P2
U 1 1 580FECA1
P 900 4500
F 0 "P2" H 900 4600 50  0000 C CNN
F 1 "CONN_01X01" V 1000 4500 50  0001 C CNN
F 2 "footprints:1pin_big" H 900 4500 50  0001 C CNN
F 3 "" H 900 4500 50  0000 C CNN
	1    900  4500
	-1   0    0    -1  
$EndComp
$Comp
L CONN_01X01 P5
U 1 1 580FECA3
P 6200 5700
F 0 "P5" H 6200 5800 50  0000 C CNN
F 1 "GND" V 6300 5700 50  0000 C CNN
F 2 "footprints:1pin_big" H 6200 5700 50  0001 C CNN
F 3 "" H 6200 5700 50  0000 C CNN
	1    6200 5700
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X01 P4
U 1 1 580FECA4
P 6200 4700
F 0 "P4" H 6200 4800 50  0000 C CNN
F 1 "-12V" V 6300 4700 50  0000 C CNN
F 2 "footprints:1pin_big" H 6200 4700 50  0001 C CNN
F 3 "" H 6200 4700 50  0000 C CNN
	1    6200 4700
	1    0    0    -1  
$EndComp
$Comp
L LED D5
U 1 1 58108DC2
P 5700 5450
F 0 "D5" H 5700 5550 50  0000 C CNN
F 1 "LED" H 5700 5350 50  0000 C CNN
F 2 "" H 5700 5450 50  0000 C CNN
F 3 "" H 5700 5450 50  0000 C CNN
	1    5700 5450
	0    1    1    0   
$EndComp
$Comp
L TRANSFO4 T1
U 1 1 5811449A
P 1850 4300
F 0 "T1" H 1850 4550 50  0000 C CNN
F 1 "TRANSFO4" H 1850 4000 50  0000 C CNN
F 2 "" H 1850 4300 50  0000 C CNN
F 3 "" H 1850 4300 50  0000 C CNN
	1    1850 4300
	-1   0    0    1   
$EndComp
$Comp
L F_Small F1
U 1 1 5811D32D
P 1250 4100
F 0 "F1" H 1210 4160 50  0000 L CNN
F 1 "F_Small" H 1130 4040 50  0000 L CNN
F 2 "" H 1250 4100 50  0000 C CNN
F 3 "" H 1250 4100 50  0000 C CNN
	1    1250 4100
	1    0    0    -1  
$EndComp
Text Notes 700  4350 0    60   ~ 0
AC Power
$Comp
L LED D4
U 1 1 581092B7
P 5700 4050
F 0 "D4" H 5700 4150 50  0000 C CNN
F 1 "LED" H 5700 3950 50  0000 C CNN
F 2 "" H 5700 4050 50  0000 C CNN
F 3 "" H 5700 4050 50  0000 C CNN
	1    5700 4050
	0    -1   -1   0   
$EndComp
Connection ~ 3300 4300
Connection ~ 5700 3300
Connection ~ 3900 4300
Connection ~ 3900 5700
Wire Wire Line
	3300 4300 3300 5700
Connection ~ 3900 4700
Connection ~ 4800 5250
Connection ~ 4300 5250
Wire Wire Line
	4800 5250 4300 5250
Wire Wire Line
	4300 5200 4300 5300
Connection ~ 4800 5700
Connection ~ 4300 5700
Wire Wire Line
	3900 5100 3900 5700
Connection ~ 4300 4700
Wire Wire Line
	2400 4700 4600 4700
Wire Wire Line
	4800 5000 4800 5300
Connection ~ 5300 5700
Wire Wire Line
	3300 5700 6000 5700
Wire Wire Line
	5300 5100 5300 5700
Connection ~ 5300 4700
Connection ~ 5300 4300
Connection ~ 5300 3300
Connection ~ 3900 3300
Connection ~ 4300 3300
Wire Wire Line
	3300 3300 4600 3300
Connection ~ 4800 3850
Connection ~ 4300 3850
Wire Wire Line
	4800 3850 4300 3850
Wire Wire Line
	4300 3800 4300 3900
Wire Wire Line
	4800 3600 4800 3900
Connection ~ 4800 4300
Wire Wire Line
	5300 4300 5300 3700
Connection ~ 4300 4300
Wire Wire Line
	2250 4300 5700 4300
Wire Wire Line
	3900 3700 3900 4300
Wire Wire Line
	3300 3600 3300 3300
Wire Wire Line
	2900 3100 2900 3200
Wire Wire Line
	2500 3600 2400 3600
Wire Wire Line
	2400 3600 2400 4700
Connection ~ 5700 5700
Wire Wire Line
	5700 3800 5700 3850
Wire Wire Line
	5700 5200 5700 5250
Wire Wire Line
	5700 5650 5700 5700
Wire Wire Line
	5000 3300 6000 3300
Wire Wire Line
	5000 4700 6000 4700
Wire Wire Line
	2250 4100 2250 3100
Wire Wire Line
	2250 3100 2900 3100
Wire Wire Line
	2250 4500 2900 4500
Wire Wire Line
	2900 4500 2900 4000
Wire Wire Line
	1450 4100 1350 4100
Wire Wire Line
	1100 4100 1150 4100
Wire Wire Line
	1450 4500 1100 4500
Connection ~ 5700 4700
Wire Wire Line
	5700 4300 5700 4250
$EndSCHEMATC
