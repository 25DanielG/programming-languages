      PROGRAM LINEC
C
C Daniel Gergov
C March 19, 2025
C
C The following counts the number of lines in a text file. It opens
C the file, reads it line by line using a function, and outputs the
C computer line count.
C
C gfortran -Wall linec.for
C

C
C PARAMETERS
C
      INTEGER UNIT
      PARAMETER (UNIT=10)

C
C LOCAL VARIABLES
C
      CHARACTER*16 FILENM
      INTEGER*8 NUMLIN

C
C FUNCTION DECLARATIONS
C
      INTEGER*8 COUNTL

C
C DATA STATEMENTS
C
      DATA FILENM /'data.txt'/

C
C MAIN PROGRAM MODULE
C
      OPEN(UNIT=UNIT, FILE=FILENM)

      WRITE(*,*) 'SUCCESS: Opening file: ', FILENM

      NUMLIN = COUNTL(UNIT)
      WRITE(*,*) 'RETURN: Number of lines: ', NUMLIN

      CLOSE(UNIT)
      STOP
      END PROGRAM

CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C FUNCTIONS
C

C 
C The COUNTL function counts the number of lines in a file. The code
C reads the file line by line until the end of the file is reached.
C The function tracks the number of lines using an INTEGER*8 and
C returns the final count. The function accepts a file unit number
C as an argument.
C
      FUNCTION COUNTL(NMUNIT)
      INTEGER*8 COUNTL
      INTEGER NMUNIT
C
      REWIND(NMUNIT)

      COUNTL = 0
10    READ(NMUNIT, *, END=20)
         COUNTL = COUNTL + 1
      GOTO 10

20    REWIND(NMUNIT)
      RETURN
      END
