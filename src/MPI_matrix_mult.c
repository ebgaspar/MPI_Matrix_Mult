#include <mpi/mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MASTER 0	// rank do processo principal
#define FROM_MASTER 1	// direcao da mensagem -> mestre - escravo
#define FROM_WORKER 2	// direcao da mensagem -> escravo - mestre

void print_matrix( const int NLA, const int NCB, const double C[ ][ NCB ] ) ;
void create_matrix( const int NLA , const int NCA , const int NCB ,
	double A[ ][ NCA ] , double B[ ][ NCB ] ) ;

int main( int argc , char **argv )
{
    int num_tasks ,			// numero total de processos
	    rank , 			//
	    num_process , 		// numero de sub-processos que executarao a multiplicacao
	    source , dest , mtype ,	// fonte, desstino e direcao da mensagem
	    rows ,			// linhas da matriz A enviada paa cada processo
	    averow , extra , offset ;	// determina quantas linhas serao enviadas para cada processo

    if ( argc < 2 )
    {
	fprintf( stderr, "Informar tamanho da matriz!\n" ) ;
	exit( 1 ) ;
    }


    int MATSIZE = atoi( argv[ 1 ] ) ; // tamanho da matriz

    int NLA = MATSIZE ;	// numero de linhas  da matriz A
    int NCA = MATSIZE ;	// numero de colunas da matriz A
    int NCB = MATSIZE ;	// numero de colunas da matriz B

    // Definicao das matrizes
    double a[ NLA ][ NCA ] ;
    double b[ NCA ][ NCB ] ;
    double c[ NLA ][ NCB ] ;

    MPI_Status status ;

    MPI_Init( &argc , &argv ) ;
    MPI_Comm_rank( MPI_COMM_WORLD , &rank ) ;
    MPI_Comm_size( MPI_COMM_WORLD , &num_tasks ) ;

    if ( num_tasks < 2 )
    {
	int rc = 0 ;
	printf( "Precisa de pelo menos 2 processos para executar!\n" ) ;
	MPI_Abort( MPI_COMM_WORLD , rc ) ;
	exit( 2 ) ;
    }

    num_process = num_tasks - 1 ; // num_process é o numero de processos que realizarao a multiplicacao

    // Processo principal
    if ( rank == MASTER )
    {
	fprintf( stdout, "Execucao com %d processos.\n\n" , num_tasks ) ;

	create_matrix( NLA, NCA, NCB, a , b ) ;

	// Inicio de medicao do tempo
	double start = MPI_Wtime( ) ;

	// Divisao das matrizes
	averow = NLA / num_process ;	// divide a quatidade de linhas pelo numero de processos
	extra = NLA % num_process ;	// pega as linhas que restaram
	offset = 0 ;
	mtype = FROM_MASTER ;

	for ( dest = 1 ; dest <= num_process ; dest++ )
	{
	    rows = ( dest <= extra ) ? averow + 1 : averow ;

	    fprintf( stdout, "Eviando %d linhas da matriz para processo %d com offset de %d\n" , rows , dest ,
		    offset ) ;

	    MPI_Send( &offset , 1 , MPI_INT , dest , mtype , MPI_COMM_WORLD ) ;
	    MPI_Send( &rows , 1 , MPI_INT , dest , mtype , MPI_COMM_WORLD ) ;
	    MPI_Send( &a[ offset ][ 0 ] , rows * NCA , MPI_DOUBLE , dest , mtype , MPI_COMM_WORLD ) ;
	    MPI_Send( &b , NCA * NCB , MPI_DOUBLE , dest , mtype , MPI_COMM_WORLD ) ;
	    offset += rows ;
	}

	/* Receive results from worker tasks */
	mtype = FROM_WORKER ;
	for ( int i = 1 ; i <= num_process ; i++ )
	{
	    source = i ;
	    MPI_Recv( &offset , 1 , MPI_INT , source , mtype , MPI_COMM_WORLD ,
		    &status ) ;
	    MPI_Recv( &rows , 1 , MPI_INT , source , mtype , MPI_COMM_WORLD ,
		    &status ) ;
	    MPI_Recv( &c[ offset ][ 0 ] , rows * NCB , MPI_DOUBLE , source ,
		    mtype ,
		    MPI_COMM_WORLD , &status ) ;

	}

	double finish = MPI_Wtime( ) ; // Fim da contagem de tempo
	fprintf( stdout, "Processo terminou em %f segundos.\n\n" , finish - start ) ;

	print_matrix( NLA, NCB , c ) ;
    }
    else // outros processes
    {
	mtype = FROM_MASTER ;

	// Recebe os dados do processo principal
	MPI_Recv( &offset , 1 , MPI_INT , MASTER , mtype , MPI_COMM_WORLD , &status ) ;
	MPI_Recv( &rows , 1 , MPI_INT , MASTER , mtype , MPI_COMM_WORLD , &status ) ;
	MPI_Recv( &a , rows * NCA , MPI_DOUBLE , MASTER , mtype , MPI_COMM_WORLD , &status ) ;
	MPI_Recv( &b , NCA * NCB , MPI_DOUBLE , MASTER , mtype , MPI_COMM_WORLD , &status ) ;

	for ( int k = 0 ; k < NCB ; k++ )
	{
	    for ( int i = 0 ; i < rows ; i++ )
	    {
		c[ i ][ k ] = 0.0 ;

		for ( int j = 0 ; j < NCA ; j++ )
		{
		    c[ i ][ k ] += a[ i ][ j ] * b[ j ][ k ] ;
		}
	    }
	}

	mtype = FROM_WORKER ;

	MPI_Send( &offset , 1 , MPI_INT , MASTER , mtype , MPI_COMM_WORLD ) ;
	MPI_Send( &rows , 1 , MPI_INT , MASTER , mtype , MPI_COMM_WORLD ) ;
	MPI_Send( &c , rows * NCB , MPI_DOUBLE , MASTER , mtype , MPI_COMM_WORLD ) ;
    }

    MPI_Finalize( ) ;
}

void print_matrix( const int NLA, const int NCB, const double C[ ][ NCB ] )
{
    for( int i = 0 ; i < NLA ; ++i )
    {
	for( int j = 0 ; j < NCB ; ++j )
	{
	    fprintf( stdout, "%.2f ", C[ i ][ j ] ) ;
	}
	fprintf( stdout, "\n" ) ;
    }
}

void create_matrix( const int NLA , const int NCA , const int NCB ,
	double A[ ][ NCA ] , double B[ ][ NCB ] )
{
    double a = (double) NCA ;

    srand( 1 );

    for ( int i = 0 ; i < NLA ; ++i )
    {
	for ( int j = 0 ; j < NCA ; ++j )
	{
	    A[ i ][ j ] = ( (double) rand( ) / (double) ( RAND_MAX ) ) * a ;
	    fprintf( stdout , "%.3f " , A[ i ][ j ] ) ;
	}
	fprintf( stdout , "\n" ) ;
    }

    fprintf( stdout , "\n" ) ;

    for ( int i = 0 ; i < NCA ; i++ )
    {
	for ( int j = 0 ; j < NCB ; j++ )
	{
	    B[ i ][ j ] = ( (double) rand( ) / (double) ( RAND_MAX ) ) * a ;
	    fprintf( stdout , "%.3f " , B[ i ][ j ] ) ;
	}
	fprintf( stdout , "\n" ) ;
    }
}
