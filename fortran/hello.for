      PROGRAM HELLO
C
C Daniel Gergov
C March 14, 2025
C
C The following code prints "HELLO WORLD!" by calling a subroutine.
C
C gfortran -Wall hello.for
C

C
C LOCAL VARIABLES
C
      CHARACTER*16 STRING

C
C DATA STATEMENTS
C
      DATA STRING /'HELLO WORLD!'/

C
C MAIN PROGRAM MODULE
C
      CALL PRINTM(STRING)
      STOP
      END PROGRAM

CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C SUBROUTINES
C

C
C Prints a given message of at most 16 bytes to the console with alpha-numeric formatting
C
      SUBROUTINE PRINTM(MSG)
      CHARACTER*16 MSG
C
      WRITE(*,10) MSG
10    FORMAT(A)
      RETURN
      END