      PROGRAM HISTO
C
C Daniel Gergov
C April 2, 2025
C
C This program reads a file containing two arrays of integers,
C calculates the mean, sample standard deviation, min, and max
C of the second array, and outputs the first and last values,
C the mean, the standard deviation, minimum value, and maximum
C value. It then creates and outputs a histogram of the second
C array of data.
C
C gfortran -Wall histo.for
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
      DATA FILENM /'Geiger01-62.txt'/

C
C MAIN PROGRAM MODULE
C
      OPEN(UNIT=UNIT, FILE=FILENM)
 
      WRITE(*,900) 'SUCCESS: Opening file: ', FILENM
900   FORMAT(A, A25)

      NUMLIN = COUNTL(UNIT)
      WRITE(*,910) 'RETURN: Number of lines:', NUMLIN
910   FORMAT(A, I10)

      CALL FLPROC(UNIT, NUMLIN)

      CLOSE(UNIT)
      STOP
      END PROGRAM

CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C SUBROUTINES
C

C
C The FLPROC subroutine processes a file containing two arrays of
C integers. The code calculates the mean, sample standard deviation,
C min, and max of the second array and outputs the first and last
C values, the mean, the standard deviation, minimum value, and maximum
C value. The subroutine then calls subroutines to create and output a
C histogram of the second array of data. The subroutine accepts a file
C unit number and the number of lines as arguments.
C
      SUBROUTINE FLPROC(NMUNIT, LINES)
      INTEGER NMUNIT
      INTEGER LINES
C
C LOCAL VARIABLES
C
      INTEGER*8 LARRAY(LINES)
      INTEGER SARRAY(LINES)
      REAL NMEAN
      REAL NSTDEV
      INTEGER MINVAL
      INTEGER MAXVAL
      INTEGER BINNUM
      CHARACTER*24 BINSTR
C
C FUNCTION DECLARATIONS
C
      REAL AVG
      REAL STDEV
C
C DATA STATEMENTS
C
      DATA BINSTR /'Enter number of bins: '/
C
      CALL FLLOAD(NMUNIT, LINES, LARRAY, SARRAY)

      WRITE(*,1000) 'RETURN: First value:', SARRAY(1)
1000  FORMAT(A, I14)
      WRITE(*,1010) 'RETURN: Last value:', SARRAY(LINES)
1010  FORMAT(A, I15)

      NMEAN = AVG(LINES, SARRAY)
      NSTDEV = STDEV(LINES, SARRAY)

      WRITE(*,1020) 'RETURN: Mean:', NMEAN
1020  FORMAT(A, F28.6)
      WRITE(*,1030) 'RETURN: Standard Deviation:', NSTDEV
1030  FORMAT(A, F13.6)

      CALL MINMAX(LINES, SARRAY, MINVAL, MAXVAL)
      WRITE(*,1050) 'RETURN: Min:', MINVAL
      WRITE(*,1050) 'RETURN: Max:', MAXVAL

      CALL PROMPT(BINSTR, BINNUM)
      IF (BINNUM < 1) THEN
         WRITE(*,1040) 'ERROR: Invalid number of bins.'
         RETURN
      END IF
      IF (BINNUM == 1) THEN
         WRITE(*,1040) 'WARNING: Only one bin will be created.'
      END IF
      CALL MKHIST(LINES, SARRAY, BINNUM, MINVAL, MAXVAL)
C
      RETURN
1040  FORMAT(A)
1050  FORMAT(A, I22)
      END

C
C The FLLOAD subroutine loads two arrays of integers from the file.
C The code reads the file line by line to store the values in two
C arrays. The subroutine accepts a file unit number, the number of
C lines, and the two arrays as arguments.
C
      SUBROUTINE FLLOAD(UNITNM, LINNUM, ARRAYL, ARRAYS)
      INTEGER UNITNM
      INTEGER LINNUM
      INTEGER*8 ARRAYL(LINNUM)
      INTEGER ARRAYS(LINNUM)
C
      INTEGER I
      DO I = 1, LINNUM
         READ(UNITNM, *) ARRAYL(I), ARRAYS(I)
      END DO
C
      RETURN
      END

C
C The MINMAX subroutine calculates the minimum and maximum values
C of an array of integers. The code iterates through the array to
C find the minimum and maximum values. The subroutine accepts the
C number of elements, the array, and the minimum and maximum values
C as arguments.
C
      SUBROUTINE MINMAX(NUMEL, ARR, MINV, MAXV)
      INTEGER NUMEL
      INTEGER ARR(NUMEL)
      INTEGER MINV
      INTEGER MAXV
C
      INTEGER I
      MINV = ARR(1)
      MAXV = ARR(1)
      DO I = 2, NUMEL
         IF (ARR(I) < MINV) MINV = ARR(I)
         IF (ARR(I) > MAXV) MAXV = ARR(I)
      END DO
C
      RETURN
      END

C
C The PROMPT subroutine prompts the user for input. The code
C displays a message and reads an integer value from the user.
C The subroutine accepts a message and a variable to store the
C input as arguments.
C
      SUBROUTINE PROMPT(MESS, VAR)
      CHARACTER*24 MESS
      INTEGER VAR
C
      WRITE(*,1100, ADVANCE='NO') MESS
1100  FORMAT(A, T33)
      READ(*,*) VAR
C
      RETURN
      END

C
C The HISTOGRAM subroutine creates a histogram of an array of
C integers. The code initializes a histogram array, calls the
C FLHIST subroutine to fill the histogram, and then calls the
C PRHIST subroutine to print the histogram. The subroutine
C accepts the number of elements, the array, the number of bins,
C the minimum value, and the maximum value as arguments.
C
      SUBROUTINE MKHIST(NUMEL, NARRAY, NBINS, MINV, MAXV)
      INTEGER NUMEL
      INTEGER NARRAY(NUMEL)
      INTEGER NBINS
      INTEGER MINV
      INTEGER MAXV
C
C LOCAL VARIABLES
C
      INTEGER*8 HISTG(NBINS)
      INTEGER I
      DO I = 1, NBINS
         HISTG(I) = 0
      END DO

      CALL FLHIST(NUMEL, NARRAY, NBINS, MINV, MAXV, HISTG)
      CALL PRHIST(NBINS, HISTG)

      RETURN
      END

C
C The FLHIST subroutine fills a histogram array with the frequency
C of values in an array of integers. The code calculates the
C index for each value in the array and increments the corresponding
C histogram bin. The subroutine accepts the number of elements,
C the array, the number of bins, the minimum value, the maximum
C value, and the histogram array as arguments.
C
      SUBROUTINE FLHIST(NELS, DATARR, NUBINS, MINV, MAXV, HIST)
      INTEGER NELS
      INTEGER DATARR(NELS)
      INTEGER NUBINS
      INTEGER MINV
      INTEGER MAXV
      INTEGER*8 HIST(NUBINS)
C
C LOCAL VARIABLES
C
      INTEGER I
      INTEGER INDEX
      REAL SCALE
C
      SCALE = REAL(NUBINS) / (MAXV - MINV + 1)

      DO I = 1, NELS
         INDEX = INT((DATARR(I) - MINV) * SCALE) + 1
         IF (INDEX > NUBINS) INDEX = NUBINS
         HIST(INDEX) = HIST(INDEX) + 1
      END DO

      RETURN
      END

C
C The PRHIST subroutine prints the histogram. The code finds the
C maximum bin value and calls the VPHIST and GPHIST subroutines
C to print the histogram values and graphical representation.
C
      SUBROUTINE PRHIST(NUBINS, HIST)
      INTEGER NUBINS
      INTEGER*8 HIST(NUBINS)
C
C LOCAL VARIABLES
C
      INTEGER*8 MAXBIN
C
      MAXBIN = 0
      DO I = 1, NUBINS
         IF (HIST(I) > MAXBIN) MAXBIN = HIST(I)
      END DO

      CALL VPHIST(NUBINS, HIST)

      CALL GPHIST(NUBINS, HIST, MAXBIN)
C
      RETURN
      END

C
C The VPHIST subroutine prints the histogram values. The code
C iterates through the histogram array and prints the bin number
C and the corresponding value. The subroutine accepts the number
C of bins and the histogram array as arguments.
C
      SUBROUTINE VPHIST(BINUM, HISTA)
      INTEGER BINUM
      INTEGER*8 HISTA(BINUM)
C
C LOCAL VARIABLES
C
      INTEGER I
C
      DO I = 1, BINUM
         WRITE(*,1200) I-1, HISTA(I)
1200     FORMAT(I3, 2X, I8)
      END DO
C
      RETURN
      END

C
C The GPHIST subroutine prints a graphical representation of the
C histogram. The code calculates the number of hash marks to print
C based on the maximum bin value and the width of the histogram.
C The subroutine accepts the number of bins, the histogram array,
C and the maximum bin value as arguments.
C
      SUBROUTINE GPHIST(BINUM, HISTA, BINMAX)
      INTEGER BINUM
      INTEGER*8 HISTA(BINUM)
      INTEGER*8 BINMAX
C
C PARAMETERS
C
      INTEGER MAXWID
      PARAMETER (MAXWID = 100)
C
C LOCAL VARIABLES
C
      INTEGER I
      INTEGER J
      INTEGER NHASH
      CHARACTER*1 HASH
C
C DATA STATEMENTS
C
      DATA HASH /'#'/
C
      WRITE(*,*)
      DO I = 1, BINUM
         NHASH = INT(REAL(HISTA(I)) / REAL(BINMAX) * MAXWID)
         WRITE(*,1210, ADVANCE='NO') I-1
1210     FORMAT(I3, 2X)
         DO J = 1, NHASH
            WRITE(*,1220, ADVANCE='NO') HASH
1220        FORMAT(A)
         END DO
         WRITE(*,*)
      END DO

      WRITE(*,1230) 'REP: # =', REAL(BINMAX) / MAXWID
1230  FORMAT(A, ES12.5)
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
C LOCAL VARIABLES
C
      REAL SUM
C
      INTEGER I
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
C LOCAL VARIABLES
C
      REAL SUM
      REAL MEAN
      REAL VAR
C
      INTEGER I
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
