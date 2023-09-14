/******************************************************
 * @file cubo.c
 * @author Marcos Zucolotto
 * @date  1 de set de 2023
 * @version 0.1
 * @brief Definicoes basicas Cubo 8x8x8
 ******************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifndef INC_CUBO_H_
#define INC_CUBO_H_

#define CAMADAS 8 //!< Quantidade de camadas
#define LINHAS  8  //!< Quantidade de linhas/camada
#define COLUNAS  8  //!< Quantidade de colunas/camada


#define NBRILHO 10 //!< niveis de brilho


typedef struct  {          //coordenada no espaco 3d
				uint8_t x;
				uint8_t y;
				uint8_t z;
				}voxel_t ;

typedef enum {TC_NADA = -10,
			  TC_NCABE, // figura nao cabe
           	  TC_FORA,      // voxel fora do cubo
			  TC_OK=0,
			  TC_TERMINOU
              }TC_StatusTypedef;




// Prototipos funcoes
void TRO_Cubo_Start_IT(TIM_HandleTypeDef *htim,SPI_HandleTypeDef *hspi);

void TRO_Cubo_Atualiza(void);
void TRO_Cubo_Controle(void);

void TRO_Cubo_Set(uint8_t valor);
TC_StatusTypedef TRO_Cubo_VoxelSet(voxel_t *vx,uint8_t x, uint8_t y, uint8_t z);

TC_StatusTypedef TRO_Cubo_LigaPonto(voxel_t *vx);
TC_StatusTypedef TRO_Cubo_ApagaPonto(voxel_t *vx);
uint8_t TRO_Cubo_LePonto(voxel_t *vx);
void TRO_Cubo_ligaPonto( voxel_t *vx);
void TRO_Cubo_apagaPonto( voxel_t *vx);
uint8_t TRO_Cubo_lePonto( voxel_t *vx);
TC_StatusTypedef TRO_Cubo_desCubo(voxel_t *vx, uint8_t tam);
TC_StatusTypedef TRO_Cubo_apagaCubo(voxel_t *vx, uint8_t tam);
void TRO_Cubo_pBaixo(void);

void TRO_Cubo_iTopo(uint8_t camadaTopo[LINHAS]);
void TRO_Cubo_pTopo(void);
void TRO_Cubo_iBaixo(uint8_t camadaBaixo[LINHAS]);
void TRO_Cubo_pFrente(void);
void TRO_Cubo_iFundo(uint8_t novoFundo[LINHAS]);
void TRO_Cubo_pFundo(void);
void TRO_Cubo_iFrente(uint8_t novaFrente[LINHAS]);
void TRO_Cubo_pDireita(void);
void TRO_Cubo_iEsquerda(uint8_t novoLado[LINHAS]);
void TRO_Cubo_pEsquerda(void);
void TRO_Cubo_iDireita(uint8_t novoLado[LINHAS]);
/*******************************************************************/

void TRO_Cubo_Chuva(uint16_t gotas,uint16_t comprimento);
TC_StatusTypedef TRO_Cubo_ChuvaN(void);
TC_StatusTypedef TRO_Cubo_Regressiva(void);
TC_StatusTypedef TRO_Cubo_CuboExpande(voxel_t *origem,uint8_t tamanho);
TC_StatusTypedef TRO_Cubo_ExpEnc(void);


/* Tempos das animacoes em ms*/
#define T_CHUVA 110
#define T_REGRESSIVA (1000/8)
#define T_EXPANDE (200)
#define T_ENCOLHE (200)
#define T_EXPENC (30)





#endif /* INC_CUBO_H_ */
