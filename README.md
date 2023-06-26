## Rodar Simulador de Caldeira

```
java -jar vendor/Aquecedor.jar
```

## Rodar o Ambiente Linux

```
docker build -t str .
```

```
docker run -it -v $(pwd)/src:/code --name str str /bin/bash
```

## Compilar e Executar o Código em C

```
gcc *.c *.h -lrt
./a.out host.docker.internal 4545
```

## Sair do Ambiente Linux

Digite o seguinte comando para também remover os arquivos `.gch` gerados após a compilação.

```
exit
```
