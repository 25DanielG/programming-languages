      PROGRAM ARRST
C
C Daniel Gergov
C March 26, 2025
C
C The following code reads an array of integers from a text file and
C outputs the number of lines, the first and last values, the mean,
C and the standard deviation. The code reads the file line by line
C to store the length, then reads the file again to load the array.
C
C gfortran -Wall arrst.for
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
      INTEGER NUMLIN

C
C FUNCTION DECLARATIONS
C
      INTEGER COUNTL

C
C DATA STATEMENTS
C
      DATA FILENM /'array.txt'/

C
C MAIN PROGRAM MODULE
C
      OPEN(UNIT=UNIT, FILE=FILENM)

      WRITE(*,*) 'SUCCESS: Opening file: ', FILENM

      NUMLIN = COUNTL(UNIT)
      WRITE(*,*) 'RETURN: Number of lines: ', NUMLIN

      CALL FLPROC(UNIT, NUMLIN)

      CLOSE(UNIT)
      STOP
      END PROGRAM

CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C SUBROUTINES
C

C
C The FLPROC subroutine processes the array of integers. The code
C calculates the mean and standard deviation of the array and outputs
C the first and last values, the mean, and the standard deviation.
C The subroutine accepts a file unit number and the number of lines
C as arguments.
C
      SUBROUTINE FLPROC(NMUNIT, LINES)
      INTEGER NMUNIT
      INTEGER LINES
C
      INTEGER ARRAY(LINES)
      REAL NMEAN
      REAL NSTDEV
C
C FUNCTION DECLARATIONS
C
      REAL AVG
      REAL STDEV
C
      CALL FLLOAD(NMUNIT, LINES, ARRAY)

      WRITE(*,*) 'RETURN: First value: ', ARRAY(1)
      WRITE(*,*) 'RETURN: Last value: ', ARRAY(LINES)

      NMEAN = AVG(LINES, ARRAY)
      NSTDEV = STDEV(LINES, ARRAY)

      WRITE(*,*) 'RETURN: Mean: ', NMEAN
      WRITE(*,*) 'RETURN: Standard Deviation: ', NSTDEV
C
      RETURN
      END

C
C The FLLOAD subroutine loads an array of integers from the file.
C The code reads the file line by line to store the values in the
C array. The subroutine accepts a file unit number, the number of
C lines, and the array as arguments.
C
      SUBROUTINE FLLOAD(UNITNM, LINNUM, ARRAY)
      INTEGER UNITNM
      INTEGER LINNUM
      INTEGER ARRAY(LINNUM)
C
      DO I = 1, LINNUM
         READ(UNITNM, *) ARRAY(I)
      END DO
C
      RETURN
      END

CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C FUNCTIONS
C

C 
C The COUNTL function counts the number of lines in a file. The code
C reads the file line by line until the end of the file is reached.
C The function tracks the number of lines using an INTEGER and
C returns the final count. The function accepts a file unit number
C as an argument.
C
      FUNCTION COUNTL(NMUNIT)
      INTEGER COUNTL
      INTEGER NMUNIT
C
      REWIND(NMUNIT)

      COUNTL = 0
10    READ(NMUNIT, *, END=20)
         COUNTL = COUNTL + 1
      GOTO 10

20    REWIND(NMUNIT)
C
      RETURN
      END


C
C The AVG function calculates the mean of an array of integers. The
C code sums the array and divides by the number of elements,
C returning the mean. The function accepts the number of elements
C and the array as arguments.
C
      FUNCTION AVG(LINNUM, ARRAY)
      REAL AVG
      INTEGER LINNUM
      INTEGER ARRAY(LINNUM)
C
      REAL SUM
C
      SUM = 0.0
      DO I = 1, LINNUM
         SUM = SUM + ARRAY(I)
      END DO
C
      AVG = SUM / LINNUM
C
      RETURN
      END

C
C The STDEV function calculates the standard deviation of an array
C of integers. The code calculates the mean of the array, then sums
C the squared differences from the mean to calculate the variance.
C The code roots the variance to calculate and return stedv. It
C accepts the number of elements and the array as arguments.
C
      FUNCTION STDEV(LINNUM, ARRAY)
      REAL STDEV
      INTEGER LINNUM
      INTEGER ARRAY(LINNUM)
C
      REAL SUM
      REAL MEAN
      REAL VAR
C
      SUM = 0.0
      DO I = 1, LINNUM
         SUM = SUM + ARRAY(I)
      END DO

      MEAN = SUM / LINNUM

      SUM = 0.0
      DO I = 1, LINNUM
         SUM = SUM + (ARRAY(I) - MEAN)**2
      END DO

      VAR = SUM / (LINNUM - 1)
C
      STDEV = SQRT(VAR)
C
      RETURN
      END
