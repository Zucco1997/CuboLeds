/**********************************************************************************
 * cubo.c
 *
 *  Created on: Sep 6, 2023
 *      Author: Zucco
 */

#include "cubo.h"
#include "main.h"
#include "fontesCubo.h"
#include "figurasCubo.h"
#include "randomAD.h"

#define BIT0 (0x01)
#define BIT7 (0x80)

#define TOPO 	 0
#define BAIXO 	(CAMADAS -1)
#define FUNDO   0
#define FRENTE  (LINHAS-1)
#define DIREITA  0
#define ESQUERDA (COLUNAS-1)



/*********************************************************************************************
 * Definicoes de hardware
 ********************************************************************************************/

#define BORDPULSO 2
typedef enum{DESCE=0,SOBE} bordas_t;

bordas_t bPulso=SOBE;				//!< indica borda que gerou a int
uint32_t tPulso[BORDPULSO]={65,60}; //!< tempos do pulso de carga paralele (RCLK)

//! Selecao da ativacao de cada camada
const uint16_t selCam[CAMADAS]={CM0_Pin,CM1_Pin,CM2_Pin,CM3_Pin,CM4_Pin,CM5_Pin,CM6_Pin,CM7_Pin};
//! Todos sinais de selecao
const uint32_t CM_ALL_Pin=(CM0_Pin|CM1_Pin|CM2_Pin|CM3_Pin|CM4_Pin|CM5_Pin|CM6_Pin|CM7_Pin);

TIM_HandleTypeDef *htimScan; //!< TIM utilizado para atualizacao
SPI_HandleTypeDef *hspiData; //!< SPI utilizada na transferencia de dados

// Indice da camada atual na varredura
uint16_t camAtual=0; //!< Camada ativa atual

/*********************************************************************************************/

//! imagem a ser exibida
uint8_t cubo[CAMADAS][LINHAS]={{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
							   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
//! Desenho de linha - indice=tamanho
const uint8_t LinhaCheia[8]={0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
//! Desenho de linha so as pontas - indice=tamanho
const uint8_t LinhaBorda[8]={0x01,0x03,0x05,0x09,0x11,0x21,0x41,0x81};

//! Mascara para selecao bit da coluna indicada pelo indice
const uint8_t maskColuna[COLUNAS]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

/*****
 *Animacoes
 */

/* Parametros fixos */
#define NGOTAS 8
#define NXCHUVA 8
#define COM_GOTA_MAX 4
#define DURACAO_GOTAS 3

//uint8_t expande=0;
//uint8_t encolhe=0;

// Prototipos privados


void copiaVoxel(voxel_t *origem,voxel_t *destino);
TC_StatusTypedef confereCoord(uint8_t x, uint8_t y, uint8_t z);
TC_StatusTypedef confereTamanho(voxel_t *vx, int8_t tamanho);
void setCamada(uint8_t cam[LINHAS],uint8_t valor);
void copiaCamada(uint8_t camOrigem[LINHAS],uint8_t camDestino[LINHAS]);
TC_StatusTypedef copiaLado(uint8_t cuboOrigem[CAMADAS][LINHAS],uint8_t ladoDestino[LINHAS],uint8_t lado);
TC_StatusTypedef colaLado(uint8_t ladoOrigem[LINHAS],uint8_t cuboDestino[CAMADAS][LINHAS],uint8_t lado);
TC_StatusTypedef copiaFace(uint8_t cuboOrigem[CAMADAS][LINHAS],uint8_t faceDestino[LINHAS],uint8_t face);
TC_StatusTypedef colaFace(uint8_t faceOrigem[LINHAS],uint8_t cuboDestino[CAMADAS][LINHAS],uint8_t face);
void copiaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],uint8_t cuboDest[CAMADAS][LINHAS]);

TC_StatusTypedef ligaPtCamada(uint8_t camada[LINHAS],uint8_t x,uint8_t y);
TC_StatusTypedef apagaPtCamada(uint8_t camada[LINHAS],uint8_t x,uint8_t y);


TC_StatusTypedef ligaPtCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx);
TC_StatusTypedef apagaPtCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx);
uint8_t lePtCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx);

TC_StatusTypedef desenhaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],voxel_t *vx, uint8_t tam);
TC_StatusTypedef apagaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],voxel_t *vx, uint8_t tam);

void deslocaBaixo(uint8_t cuboDestino[CAMADAS][LINHAS], uint8_t novaCamada[LINHAS]);
void deslocaTopo(uint8_t cuboDestino[CAMADAS][LINHAS], uint8_t novaCamada[LINHAS]);
void deslocaCamFrente(uint8_t camada[LINHAS],uint8_t novaLinha);
void deslocaCamFundo(uint8_t camada[LINHAS],uint8_t novaLinha);
void deslocaCamDireita(uint8_t camada[LINHAS], uint8_t novoLado[LINHAS]);
void deslocaCamEsquerda(uint8_t camada[LINHAS],uint8_t novoLado[LINHAS]);



/*******************************************************************************************
 * Implementacao das funcoes
 ********************************************************************************************/
/*******************************************************************
		Controle da varredura
*******************************************************************/
/***
 * @brief Configura e variaveis do modulo e inicia varredura
 * @param handler do TIM utilizado na varredura
 * @param handler da SPI utilizada na transferencia de dados
 ****/
void TRO_Cubo_Start_IT(TIM_HandleTypeDef *htim,SPI_HandleTypeDef *hspi)
{
	htimScan=htim;
	hspiData=hspi;
	  camAtual=0;
	  __HAL_TIM_SET_COMPARE(htimScan,TIM_CHANNEL_2,tPulso[bPulso]);
	  // Ativa geracao do pulso
	  HAL_TIM_PWM_Start_IT(htimScan, TIM_CHANNEL_2);
	  // Dispara temporizacao da varredura
	  HAL_TIM_Base_Start_IT(htimScan);
}
/***
 * @brief Dispara a transferencia de dados para acionamento dos leds
 * Inserir esta funcao no callback periodico do TIM
 ****/
void TRO_Cubo_Atualiza(void)
{
	if(++camAtual>=CAMADAS)
	{
		camAtual=0;
	}
	HAL_SPI_Transmit_DMA(hspiData, &(cubo[camAtual]), LINHAS);
}

/***
 * @brief Dispara a transferencia de dados para acionamento dos leds
 * Inserir esta funcao no callback OC do TIM
 ****/
void TRO_Cubo_Controle(void)
{
		if(bPulso==SOBE)
		{
		bPulso=DESCE;
		// Desliga sinais controle
		HAL_GPIO_WritePin(CM0_GPIO_Port,CM_ALL_Pin ,GPIO_PIN_RESET);
		}
	else
		{
		bPulso=SOBE;
		// Ativa camada configurada
		HAL_GPIO_WritePin(CM0_GPIO_Port,selCam[camAtual],GPIO_PIN_SET);
	  	}
	 __HAL_TIM_SET_COMPARE(htimScan,TIM_CHANNEL_2,tPulso[bPulso]);
}

/*******************************************************************
		funcoes basicas dados
*******************************************************************/
/*******************************************************************
	    voxel
*******************************************************************/
/***
 * @brief Copia variavel tipo voxel
 * @param voxel origem
 * @param voxel destino
 ****/
void copiaVoxel(voxel_t *origem,voxel_t *destino)
{
	destino->x=origem->x;
	destino->y=origem->y;
	destino->z=origem->z;
}

/***
 * @brief verifica se coordenadas pertencem ao cubo
 * @param valor coordenada x
 * @param valor coordenada y
 * @param valor coordenada z
 * @return erro se coordenadas fora do cubo
 ****/
TC_StatusTypedef confereCoord(uint8_t x, uint8_t y, uint8_t z)
{
	if (x > (COLUNAS-1) || y > (LINHAS-1) ||y > (CAMADAS-1) )
		return TC_FORA;
	return TC_OK;
}
/***
 * @brief insere valores na variavel tipo voxel
 * @param voxel destino
 * @param valor coordenada x
 * @param valor coordenada y
 * @param valor coordenada z
 * @return erro se coordenadas fora do cubo
 ****/
TC_StatusTypedef TRO_Cubo_VoxelSet(voxel_t *vx,uint8_t x, uint8_t y, uint8_t z)
{
	TC_StatusTypedef status;
	status=confereCoord(x,y,z);
	if (status==TC_OK )
	{
		vx->x=x;
		vx->y=y;
		vx->z=z;
	}
	return status;
}
/***
 * @brief confere se cabe no cubo
 * @param voxel aresta inicio
 * @param tamanho da figura
 * @return erro se coordenadas fora do cubo
 ****/
TC_StatusTypedef confereTamanho(voxel_t *vx, int8_t tamanho)
{
	TC_StatusTypedef status;
	status=confereCoord(vx->x,vx->y,vx->z);

	if (status==TC_OK )
	{
		tamanho--;
		status=confereCoord(vx->x+tamanho,vx->y+tamanho,vx->z+tamanho);
		if (status!=TC_OK)
			status=TC_NCABE;
	}
	if (tamanho<=0)
			status=TC_NADA;

	return status;
}

/*******************************************************************
    camada
*******************************************************************/
/***
 * @brief Escreve valor em todas as linhas da camada
 * @param valor de cada linha de cada camada
 */
void setCamada(uint8_t cam[LINHAS],uint8_t valor)
{
uint16_t lin;
  for (lin=0;lin<LINHAS;lin++)
 	  {
	  cam[lin]=valor;
	  }
}
/***
 * @brief copia camada (plano Z)
 * @param origem dados
 * @param destino dados
 */
void copiaCamada(uint8_t camOrigem[LINHAS],uint8_t camDestino[LINHAS])
{
uint16_t lin;
  	  for (lin=0;lin<LINHAS;lin++)
  	  {
  		  camDestino[lin]=camOrigem[lin];
  	  }
}
/***
 * @brief copia lado (plano y)
 * @param cubo origem dados
 * @param lado destino dados
 * @param indice lado
 */
TC_StatusTypedef copiaLado(uint8_t cuboOrigem[CAMADAS][LINHAS],uint8_t ladoDestino[LINHAS],uint8_t lado)
{
int16_t cam,lin;
uint8_t bit;

	if (lado >=COLUNAS)
		return TC_FORA;

	for(cam=0;cam<CAMADAS;cam++)
	{
	    ladoDestino[cam]=0;
		for(lin=0;lin<LINHAS;lin++)
		{
		  bit=cuboOrigem[cam][lin] & maskColuna[lado];
		  bit<<=lin;
		  ladoDestino[cam]|=bit;
		}
	}
	return TC_OK;
}




/***
 * @brief insere lado no ponto x  solicitado
 * @param lado origem dados
 * @param cubo destino dados
 * @param indice lado
 */
TC_StatusTypedef colaLado(uint8_t ladoOrigem[LINHAS],uint8_t cuboDestino[CAMADAS][LINHAS],uint8_t lado)
{
int16_t cam,lin;
uint8_t bit;

	if (lado >=COLUNAS)
		return TC_FORA;

	for(cam=0;cam<CAMADAS;cam++)
	{
		for(lin=0;lin<LINHAS;lin++)
		{
		  cuboDestino[cam][lin]&=~maskColuna[lado];
		  bit=ladoOrigem[lin]&maskColuna[lado];
		  cuboDestino[cam][lin]|=bit;
		}
	}
	return TC_OK;
}
/***
 * @brief copia face (plano x)
 * @param cubo origem dados
 * @param face destino dados
 * @param indice face
 */
TC_StatusTypedef copiaFace(uint8_t cuboOrigem[CAMADAS][LINHAS],uint8_t faceDestino[LINHAS],uint8_t face)
{
int16_t cam;

	if(face >=LINHAS)
		return TC_FORA;

	for(cam=0;cam<CAMADAS;cam++)
	{
		faceDestino[cam]=cuboOrigem[cam][face];
	}
	return TC_OK;
}

/***
 * @brief insere face (plano x) no ponto y (face) especificado
 * @param cubo origem dados
 * @param face destino dados
 * @param indice face
 */
TC_StatusTypedef colaFace(uint8_t faceOrigem[LINHAS],uint8_t cuboDestino[CAMADAS][LINHAS],uint8_t face)
{
int16_t cam;

	if(face >=LINHAS)
		return TC_FORA;

	for(cam=0;cam<CAMADAS;cam++)
	{
		cuboDestino[cam][face]=faceOrigem[cam];
	}
	return TC_OK;
}

/*******************************************************************
	    cubo
*******************************************************************/
/***
 * @brief Copia matriz cubo
 * @param cubo origem dados
 * @param cubo destino dados
 ****/
void copiaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],uint8_t cuboDest[CAMADAS][LINHAS])
{
	uint16_t cam,lin,col;
	for (cam=0;cam<CAMADAS;cam++)
	{
		for (lin=0;lin<LINHAS;lin++)
		{
			for (col=0;col<COLUNAS;col++)
				cuboDest[cam][lin]=cuboOrig[cam][lin];
		}
	}
}
/****
 * @brief Escreve valor em todas as linhas do cubo
 * @param matriz do cubo
 * @param valor de cada linha de cada camada
 */
void TRO_Cubo_Set(uint8_t valor)
{
	uint16_t cam,lin;
	  for (cam=0;cam<CAMADAS;cam++)
		  {
			  for (lin=0;lin<LINHAS;lin++)
				  cubo[cam][lin]=valor;
		  }
}

/*******************************************************************
    Desenho
*******************************************************************/
/****
 * @brief ativa ponto na camada
 * @param vetor camada
 * @param coordenadas
 *
 */
TC_StatusTypedef ligaPtCamada(uint8_t camada[LINHAS],uint8_t x,uint8_t y)
{
TC_StatusTypedef status;
	status = confereCoord(x, y,0);
	  if (status == TC_OK)
		  camada[y]|=maskColuna[x];
	 return status;
}

/****
 * @brief desliga ponto na camada
 * @param vetor camada
 * @param coordenadas
 *
 */
TC_StatusTypedef apagaPtCamada(uint8_t camada[LINHAS],uint8_t x,uint8_t y)
{
TC_StatusTypedef status;
	status = confereCoord(x, y,0);
	  if (status == TC_OK)
		  camada[y]&=~maskColuna[x];
	 return status;
}
/****
 * @brief liga ponto no cubo
 * @param vetor camada
 * @param coordenadas
 *
 */
TC_StatusTypedef ligaPtCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx)
{
  TC_StatusTypedef status;
  status = confereCoord(vx->x,vx->y,vx->z);
  if(status == TC_OK)
  	  cuboOrig[vx->z][vx->y]|= maskColuna[vx->x];
  return status;
}
TC_StatusTypedef apagaPtCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx)
{
	  TC_StatusTypedef status;
	  status = confereCoord(vx->x,vx->y,vx->z);
	  if(status == TC_OK)
	  	  cuboOrig[vx->z][vx->y]&=~maskColuna[vx->x];
	  return status;
}
uint8_t lePontoCubo(uint8_t cuboOrig[CAMADAS][LINHAS], voxel_t *vx)
{
	if(confereCoord(vx->x,vx->y,vx->z) != TC_OK)
		return -1;

	return ((cuboOrig[vx->z][vx->y]&maskColuna[vx->x])>>(vx->x));
}


/****
 * @brief liga ponto no cubo
 * @param coordenadas
 */
TC_StatusTypedef TRO_Cubo_LigaPonto(voxel_t *vx)
{
	return  ligaPtCubo(cubo, vx);

}
/****
 * @brief apaga ponto no cubo
 * @param coordenadas
 *
 */
TC_StatusTypedef TRO_Cubo_ApagaPonto(voxel_t *vx)
{
	  return apagaPtCubo(cubo,vx);
}
/****
 * @brief le valor do ponto no cubo
 * @param coordenadas
 *
 */
uint8_t TRO_Cubo_LePonto(voxel_t *vx)
{
	return lePontoCubo(cubo,vx);
}

/****
 * @brief desenha cubo no cubo
 * @param matriz cubo
 * @param coordenadas aresta
 * @param tamanho
 *
 */

TC_StatusTypedef desenhaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],voxel_t *vx, uint8_t tam)
{
	uint16_t i;

	TC_StatusTypedef status;
	status=confereTamanho(vx,tam);
	if(status!=TC_OK)
		return status;

	  tam--;

	  cuboOrig[vx->z][vx->y]|= (LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z][vx->y+tam]|= (LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z+tam][vx->y]|= (LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z+tam][vx->y+tam]|= (LinhaCheia[tam]<<vx->x);

	  for (i=1;i<tam;i++)
	  {
		  cuboOrig[vx->z][vx->y+i]|= (LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+tam][vx->y+i]|=(LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+i][vx->y]|=(LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+i][vx->y+tam]|=(LinhaBorda[tam]<<vx->x);
	  }
	  return status;
}

TC_StatusTypedef apagaCubo(uint8_t cuboOrig[CAMADAS][LINHAS],voxel_t *vx, uint8_t tam)
{
	uint16_t i;

	TC_StatusTypedef status;
	status=confereTamanho(vx,tam);
	if(status!=TC_OK)
		return status;

	  tam--;

	  cuboOrig[vx->z][vx->y]&=~(LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z][vx->y+tam]&=~(LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z+tam][vx->y]&=~(LinhaCheia[tam]<<vx->x);
	  cuboOrig[vx->z+tam][vx->y+tam]&=~(LinhaCheia[tam]<<vx->x);

	  for (i=1;i<tam;i++)
	  {
		  cuboOrig[vx->z][vx->y+i]&=~(LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+tam][vx->y+i]&=~(LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+i][vx->y]&=~(LinhaBorda[tam]<<vx->x);
		  cuboOrig[vx->z+i][vx->y+tam]&=~(LinhaBorda[tam]<<vx->x);
	  }
	  return status;
}


TC_StatusTypedef TRO_Cubo_desCubo(voxel_t *vx, uint8_t tam)
{
	return desenhaCubo(cubo,vx,tam);
}

TC_StatusTypedef TRO_Cubo_apagaCubo(voxel_t *vx, uint8_t tam)
{
	return apagaCubo(cubo,vx,tam);
}
/*******************************************************************
    Acao e Movimentacao
*******************************************************************/
/****
 * @brief desloca cubo para baixo, inserindo nova camada fundo
 * @param camada
 * @param linha a ser inserida
 *
 */
void deslocaBaixo(uint8_t cuboDestino[CAMADAS][LINHAS], uint8_t novaCamada[LINHAS])
{
int16_t cam;
	for (cam=(CAMADAS-1);cam>0;cam--)
	{
	 copiaCamada(cuboDestino[cam-1],cuboDestino[cam]);
	}
	copiaCamada(novaCamada,cuboDestino[TOPO]);
}
/****
 * @brief desloca cubo para topo, inserindo nova camada embaixo
 * @param cubo
 * @param camada a ser inserida
 */

void deslocaTopo(uint8_t cuboDestino[CAMADAS][LINHAS], uint8_t novaCamada[LINHAS])
{
int16_t cam;
	for (cam=0;cam<(CAMADAS-1);cam++)
	{
	 copiaCamada(cuboDestino[cam+1],cuboDestino[cam]);
	}
	copiaCamada(novaCamada,cuboDestino[BAIXO]);
}

/****
 * @brief desloca uma camada para frente, inserindo nova linha fundo
 * @param camada
 * @param linha a ser inserida
 *
 */
void deslocaCamFrente(uint8_t camada[LINHAS],uint8_t novaLinha)
{
int16_t lin;
	for (lin=(LINHAS-1);lin>0;lin--)
	{
		camada[lin]=camada[lin-1];
	}
	camada[FUNDO]=novaLinha;
}

/****
 * @brief desloca uma camada para trás, inserindo nova linha na frente
 * @param camada
 * @param linha a ser inserida
 *
 */

void deslocaCamFundo(uint8_t camada[LINHAS],uint8_t novaLinha)
{
int16_t lin;
	for (lin=0;lin<(LINHAS-1);lin++)
	{
		camada[lin]=camada[lin+1];
	}
	camada[FRENTE]=novaLinha;
}

/****
 * @brief desloca uma camada lado para direita,  inserindo nova coluna  na esquerda
 * @param camada
 * @param coluna a ser inserida
 *
 */

void deslocaCamDireita(uint8_t camada[LINHAS], uint8_t novoLado[LINHAS])
{
int16_t lin;
	for (lin=0;lin<LINHAS;lin++)
	{
		camada[lin]>>=1;
	}
	colaLado(novoLado,cubo,ESQUERDA);
}

/****
 * @brief desloca uma camada lado para esquerda,  inserindo nova coluna  na direita
 * @param camada
 * @param coluna a ser inserida
 *
 */

void deslocaCamEsquerda(uint8_t camada[LINHAS], uint8_t novoLado[LINHAS])
{
int16_t lin;

	for (lin=0;lin<LINHAS;lin++)
	{
		camada[lin]<<=1;
	}
	colaLado(novoLado,cubo,DIREITA);
}
/****
 * @brief desloca cubo para baixo, rotacionando
 *
 */
void TRO_Cubo_pBaixo(void)
{
 uint8_t camadaTopo[LINHAS];
	copiaCamada(cubo[BAIXO],camadaTopo);
	deslocaBaixo(cubo,camadaTopo);
}

/****
 * @brief desloca cubo para baixo, inserindo nova camada
 * @param camada topo
 */

void TRO_Cubo_iTopo(uint8_t camadaTopo[LINHAS])
{
 	deslocaBaixo(cubo,camadaTopo);
}

/****
 * @brief desloca cubo para cima, rotacionando
 * @param camada
 * @param coluna a ser inserida
 *
 */
void TRO_Cubo_pTopo(void)
{
uint8_t camadaBaixo[LINHAS];
	copiaCamada(cubo[TOPO],camadaBaixo);
	deslocaTopo(cubo,camadaBaixo);
}
/****
 * @brief desloca cubo para cima, inserindo nova camada
 * @param camada baixo
 */
void TRO_Cubo_iBaixo(uint8_t camadaBaixo[LINHAS])
{
	deslocaTopo(cubo,camadaBaixo);
}

/****
 * @brief desloca cubo para frente,rotacionando

 */
void TRO_Cubo_pFrente(void)
{
int16_t cam;
uint8_t linExtra;
	for (cam=0;cam<CAMADAS;cam++)
	{
		linExtra=cubo[cam][FUNDO];
		deslocaCamFrente(cubo[cam],linExtra);
	}
}
/****
 * @brief desloca cubo para frente, inserindo novo fundo
 * @param fundo a inserir
 */
void TRO_Cubo_iFundo(uint8_t novoFundo[LINHAS])
{
	int16_t cam;
		for (cam=0;cam<CAMADAS;cam++)
		{
			deslocaCamFrente(cubo[cam],novoFundo[cam]);
		}
}
/****
 * @brief desloca cubo para o fundo,rotacionando

 */
void TRO_Cubo_pFundo(void)
{
	int16_t cam;
	uint8_t linExtra;
	for (cam=0;cam<CAMADAS;cam++)
	{
		linExtra=cubo[cam][FRENTE];
		deslocaCamFundo(cubo[cam],linExtra);
	}
}
/****
 * @brief desloca cubo para o fundo, inserindo novo frente
 * @param frente a inserir
 */
void TRO_Cubo_iFrente(uint8_t novaFrente[LINHAS])
{
	int16_t cam;
	for (cam=0;cam<CAMADAS;cam++)
	{
		deslocaCamFundo(cubo[cam],novaFrente[cam]);
	}
}
/****
 * @brief desloca cubo para direita, inserindo novo frente
 * @param frente a inserir
 */
void TRO_Cubo_pDireita(void)
{
int16_t cam;
uint8_t novoLado[LINHAS];
	copiaLado(cubo,novoLado,DIREITA);
	for (cam=0;cam<CAMADAS;cam++)
	{
		deslocaCamDireita(cubo[cam],novoLado[cam]);
	}
}
/****
 * @brief desloca cubo para esquerda, rotacionando
 */
void TRO_Cubo_pEsquerda(void)
{
int16_t cam;
uint8_t novoLado[LINHAS];
	copiaLado(cubo,novoLado,ESQUERDA);
	for (cam=0;cam<CAMADAS;cam++)
	{
		deslocaCamEsquerda(cubo[cam],novoLado[cam]);
	}
}

/****
 * @brief desloca cubo para esquerda, inserindo novo lado
 * @param lado a inserir
 */
void TRO_Cubo_iEsquerda(uint8_t novoLado[LINHAS])
{
int16_t cam;
	for (cam=0;cam<CAMADAS;cam++)
	{
		deslocaCamDireita(cubo[cam],novoLado[cam]);
	}
}

/****
 * @brief desloca cubo para direita, inserindo novo lado
 * @param lado a inserir
 */
void TRO_Cubo_iDireita(uint8_t novoLado[LINHAS])
{
int16_t cam;
	for (cam=0;cam<CAMADAS;cam++)
	{
		deslocaCamEsquerda(cubo[cam],novoLado[cam]);
	}
}

/*******************************************************************
Animacoes
*******************************************************************/
void TRO_Cubo_Chuva(uint16_t gotas,uint16_t comprimento)
{
	uint8_t i;
	uint8_t nGotas=Randomf(1,gotas);
	static int8_t duracao=1;
	static uint8_t camExtra[LINHAS];

	if (comprimento>COM_GOTA_MAX)
		comprimento=COM_GOTA_MAX;

	if (--duracao)
	{
		setCamada(camExtra,0);
		for ( i = 0; i < nGotas; i++) {
			ligaPtCamada(camExtra,Random(LINHAS),Random(COLUNAS));
		}
		duracao=Randomf(1,comprimento);
	}

    TRO_Cubo_iTopo(camExtra);
}

TC_StatusTypedef TRO_Cubo_ChuvaN(void)
{
	static uint16_t conta=NXCHUVA;
	TC_StatusTypedef status=TC_OK;


	TRO_Cubo_Chuva(NGOTAS,DURACAO_GOTAS);
	if (--conta==0)
	{
		conta=NXCHUVA;
		status=TC_TERMINOU;
	}
	return status;
}




TC_StatusTypedef TRO_Cubo_Regressiva(void)
{
	static int8_t contagem=9;
	static int8_t estagio=0;
	TC_StatusTypedef status;
	static uint8_t camExtra[LINHAS];
	status=TC_OK;

	switch(estagio)
	{
	case 0:
		copiaCamada(digito[contagem],camExtra);
		//CamadaSet(camExtra,0);
		break;
	case 1:
		setCamada(camExtra,0);
		break;
	case 7:
		estagio=-1;
		if(--contagem<0)
		{
			contagem=9;
			status=TC_TERMINOU;
		}
	}

	TRO_Cubo_iFrente(camExtra);
	estagio++;
	return status;
}


TC_StatusTypedef TRO_Cubo_CuboExpande(voxel_t *origem,uint8_t tamanho)
{
	static voxel_t canto,cantoAnterior;
	static uint8_t tam;
	static uint8_t expande=0;
	TC_StatusTypedef status;
	status=confereTamanho(origem,tamanho);

	if (status!=TC_OK)
		return status;

	if (expande==0)
	{
		tam=tamanho;
		copiaVoxel(origem,&canto);
		TRO_Cubo_desCubo(&canto,tam);
		expande=1;
	}
	else
	{
		copiaVoxel(&canto,&cantoAnterior);
		canto.x--;
		canto.y--;
		canto.z--;
		tam+=2;
		status=confereTamanho(&canto,tam);
		if (status==TC_OK)
		{
		  TRO_Cubo_apagaCubo(&cantoAnterior,tam-2);
		  TRO_Cubo_desCubo(&canto,tam);
		  status=TC_OK;
		}
		else
		{
		 expande=0;
		 status=TC_TERMINOU;
		}
	}

	return status;
}

TC_StatusTypedef TRO_Cubo_CuboEncolhe(voxel_t *origem,uint8_t tamanho)
{
	static voxel_t canto,cantoAnterior;
	static int8_t tam;
	static uint8_t encolhe=0;
	TC_StatusTypedef status;
	status=confereTamanho(origem,tamanho);

	if (status!=TC_OK)
		return status;

	if (encolhe==0)
	{
		tam=tamanho;
		copiaVoxel(origem,&canto);
		TRO_Cubo_desCubo(&canto,tam);
		encolhe=1;
	}
	else
	{
		copiaVoxel(&canto,&cantoAnterior);
		canto.x++;
		canto.y++;
		canto.z++;
		tam-=2;
		status=confereTamanho(&canto,tam);
		if (status==TC_OK)
		{
		  TRO_Cubo_apagaCubo(&cantoAnterior,tam+2);
		  TRO_Cubo_desCubo(&canto,tam);
		}
		else
		{
		 encolhe=0;
		 status=TC_TERMINOU;
		}
	}

	return status;
}

TC_StatusTypedef TRO_Cubo_ExpEnc(void)
{
static voxel_t canto;
static uint8_t etapa=0;
TC_StatusTypedef stAni;

	switch(etapa)
	{
	case 0:
		TRO_Cubo_VoxelSet(&canto,3,3,3);
		etapa++;
	case 1:
		stAni=TRO_Cubo_CuboExpande(&canto,2);
		if (stAni==TC_TERMINOU)
			{
			etapa++;
			}
		stAni=TC_OK;
		break;
	case 2:
		TRO_Cubo_VoxelSet(&canto,0,0,0);
		etapa++;
	case 3:
		stAni=TRO_Cubo_CuboEncolhe(&canto,8);
		if (stAni==TC_TERMINOU)
			{
			etapa=0;
		}
		break;
	default:
		etapa=0;
	}// fim switch

	return stAni;
}
