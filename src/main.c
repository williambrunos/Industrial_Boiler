//Defini��o de Bibliotecas
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "socket.h"
#include "sensores.h"
#include "tela.h"
#include "bufduplo.h"
#include "nivelBufduplo.h"
#include "referenciaT.h"
#include "referenciaH.h"

#define	NSEC_PER_SEC    (1000000000) 	// Numero de nanosegundos em um segundo

#define N_AMOSTRAS 1000

void thread_mostra_status (void){
	double t, h;
	while(1){
		t = sensor_get("t");
		h = sensor_get("h");
		aloca_tela();
		system("tput reset");
		printf("---------------------------------------\n");
		printf("Temperatura (T)--> %.2lf\n", t);
		printf("Nivel       (H)--> %.2lf\n", h);
		printf("---------------------------------------\n");
		libera_tela();
		sleep(1);
		//								
	}	
		
}


void thread_le_sensor (void){ //Le Sensores periodicamente a cada 10ms
	struct timespec t;
	long periodo = 10e6; //10e6ns ou 10ms
	
	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC ,&t);
	while(1){
		// Espera ateh inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		
		//Envia mensagem via canal de comunica��o para receber valores de sensores		
		sensor_put(msg_socket("st-0"), msg_socket("sh-0"), msg_socket("sti0"));
		
		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}		
	}		
}

void thread_alarme (void){
	while(1){
		sensor_alarmeT(29);
		aloca_tela();
		printf("ALARME\n");
		libera_tela();
		sleep(1);	
	}
		
}

void thread_controle_nivel_agua(void) {
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim;
	long periodo = 50e6; //50ms

	double na, ni, nf;
	double proporcao, fluxo_maximo_entrada = 100.0;

	double altura, ref_altura;
	double temp_na = 80.0, temp_ni, temp, ref_temp;

	clock_gettime(CLOCK_MONOTONIC ,&t);
	t.tv_sec++;

	while(1) {
		// Espera até inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

		na = 0.0; 
		ni = 0.0; 
		nf = 0.0;

		ref_temp = ref_getT();
		ref_altura = ref_getH();

		temp = sensor_get("t");
		altura = sensor_get("h");
		temp_ni = sensor_get("ti");

		int precisa_subir_nivel = ref_altura > altura;
		int precisa_descer_nivel = ref_altura < altura;

		/*
		 * A theread de controle de temperatura 
		 * aument o nivel de agua quando a temperatura
		 * esta abaixo do valor de referencia
		 */
		double limiar_de_temperatura = 1.5;
		double limiar_de_altura = 0.1;
		int altura_esta_no_limiar = fabs(ref_altura - altura) <= limiar_de_altura;
		int temperatura_esta_no_limiar = fabs(ref_temp - temp) <= limiar_de_temperatura;

		// Determina quem está mais longe do limiar, proporcionamente
		double distancia_do_limiar_altura = fabs(ref_altura - altura) / limiar_de_altura;
		double distancia_do_limiar_temperatura = fabs(ref_temp - temp) / limiar_de_temperatura;
		int temperatura_esta_mais_longe_referencia = distancia_do_limiar_temperatura > distancia_do_limiar_altura;
		int precisa_esquentar = ref_temp > temp && (!temperatura_esta_no_limiar || temperatura_esta_mais_longe_referencia);

		if (precisa_esquentar) {
			int condicao_controle_temperatura = (ref_temp - temp) * 20 > 10.0;
			if (!condicao_controle_temperatura) {
				na = (ref_temp - temp) * 20;
				nf = na;
			}
		} else {
			if (precisa_subir_nivel) {
				// Fazer mistura de agua fria e quente para injetar água 
				// a temperatura desejada no tanque
				proporcao = (ref_temp - temp_ni) / (temp_na - temp_ni);
				na = fluxo_maximo_entrada / (1 + proporcao);
				ni = fluxo_maximo_entrada - na;
				nf = 0.0;
			} else if (precisa_descer_nivel) {
				na = 0.0;
				nf = 20.0;
			}
		}

		sprintf(msg_enviada, "ani%lf", ni);
		msg_socket(msg_enviada);
		
		sprintf(msg_enviada, "anf%lf", nf);
		msg_socket(msg_enviada);
		
		sprintf(msg_enviada, "ana%lf", na);
		msg_socket(msg_enviada);

		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC ,&t_fim);
		
		// Calcula o tempo de resposta observado em microsegundos
		atraso_fim = 1000000*(t_fim.tv_sec - t.tv_sec)   +   (t_fim.tv_nsec - t.tv_nsec)/1000;
		
		nivel_bufduplo_insereLeitura(atraso_fim);
		
		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}
	}
}

///Controle
void thread_controle_temperatura (void){
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim;
	long periodo = 50e6; //50ms
	double temp, ref_temp;
	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC ,&t);
	t.tv_sec++;
	while(1){
		
		// Espera ateh inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		
		temp = sensor_get("t");
		ref_temp = ref_getT();
		double na;
		
    	if(temp > ref_temp) { //diminui temperatura
	       	        
			sprintf( msg_enviada, "ani%lf", 100.0);
	        msg_socket(msg_enviada);
	        
			sprintf( msg_enviada, "anf%lf", 100.0);
	        msg_socket(msg_enviada);
			
			sprintf( msg_enviada, "ana%lf", 0.0 );
			msg_socket(msg_enviada);
        }

        
        if(temp < ref_temp) {    //aumenta temperatura
	     
	        if((ref_temp-temp)*20>10.0)
	        	na=10.0;
	        else
	        	na = (ref_temp-temp)*20;
					
			sprintf( msg_enviada, "ani%lf", 0.0);
	        msg_socket(msg_enviada);
			
			sprintf( msg_enviada, "anf%lf", 10.0);
	        msg_socket(msg_enviada);
			
	        sprintf( msg_enviada, "ana%lf", na);
			msg_socket(msg_enviada);
        }
        
		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC ,&t_fim);
		
		// Calcula o tempo de resposta observado em microsegundos
		atraso_fim = 1000000*(t_fim.tv_sec - t.tv_sec)   +   (t_fim.tv_nsec - t.tv_nsec)/1000;
		
		bufduplo_insereLeitura(atraso_fim);
		
		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}

		
	}
}

void thread_grava_temp_resp(void){
	FILE* dados_f;
	dados_f = fopen("dados.txt", "w");
    if(dados_f == NULL){
        printf("Erro, nao foi possivel abrir o arquivo\n");
        exit(1);    
    }
	int amostras = 1;	
	while(amostras++<=N_AMOSTRAS/200){
		long * buf = bufduplo_esperaBufferCheio();		
		int n2 = tamBuf();
		int tam = 0;
		while(tam<n2)
			fprintf(dados_f, "%4ld\n", buf[tam++]); 
		fflush(dados_f);
		aloca_tela();
		printf("Gravando no arquivo...\n");
		
		libera_tela();
						
	}
	
	fclose(dados_f);	
}


void thread_grava_tempo_resposta_controlador_nivel(void){
	FILE* dados_f;
	dados_f = fopen("tempoRespostaControleTemp.txt", "w");
    if(dados_f == NULL){
        printf("Erro, nao foi possivel abrir o arquivo\n");
        exit(1);    
    }
	int amostras = 1;	
	while(amostras++ <= N_AMOSTRAS / 200){
		
		long * buf = nivel_bufduplo_esperaBufferCheio();		
		int n2 = nivel_tamBuf();
		int tam = 0;
		while(tam<n2) {
			fprintf(dados_f, "%4ld\n", buf[tam++]); 
		}
		fflush(dados_f);

		// if (buf == NULL) {
		// 	fprintf(dados_f, "Erro ao ler buffer duplo\n");
		// 	fflush(dados_f);

		// 	printf("Erro ao ler buffer duplo\n");
		// } else {
		// 	for (int i = 0; i < n2; i++) {
		// 		printf("%4ld\n", buf[i]);
		// 		// fprintf(dados_f, "%4ld\n", buf[i]);
		// 	}

		// }


		// fflush(dados_f);

		aloca_tela();
		printf("Gravando no arquivo (nível)...\n");
		libera_tela();

	}
	
	fclose(dados_f);	
}


int main( int argc, char *argv[]) {
    ref_putT(29.0);
	ref_putH(2.0);
	cria_socket(argv[1], atoi(argv[2]) );
    
	pthread_t t1, t2, t3, t4, t5, t6, t7;
    
    pthread_create(&t1, NULL, (void *) thread_mostra_status, NULL);
    pthread_create(&t2, NULL, (void *) thread_le_sensor, NULL);
    pthread_create(&t3, NULL, (void *) thread_alarme, NULL);
    pthread_create(&t4, NULL, (void *) thread_controle_temperatura, NULL);
    pthread_create(&t5, NULL, (void *) thread_grava_temp_resp, NULL);
	pthread_create(&t6, NULL, (void *) thread_controle_nivel_agua, NULL);
	pthread_create(&t7, NULL, (void *) thread_grava_tempo_resposta_controlador_nivel, NULL);
    
	pthread_join( t1, NULL);
	pthread_join( t2, NULL);
	pthread_join( t3, NULL);
	pthread_join( t4, NULL);
	pthread_join( t5, NULL);
	pthread_join( t6, NULL);
	pthread_join( t7, NULL);
	
	return 0;
}

//teste