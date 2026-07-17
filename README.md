# MiniFileSync

MiniFileSync es un proyecto desarrollado en lenguaje C que representa un sistema básico de sincronización de archivos. Su función principal es revisar un directorio, detectar archivos nuevos o modificados y copiarlos al directorio `backup/`. Para el desarrollo del proyecto se utilizaron procesos, pipes, memoria compartida, semáforos y un proceso Logger.

## Funcionamiento general

El programa utiliza un proceso Monitor que revisa el directorio indicado cada 5 segundos. Cuando el Monitor encuentra un archivo nuevo o detecta que cambió su tamaño o fecha de modificación, asigna la copia a uno de los dos procesos Workers. Cada Worker recibe la ruta del archivo mediante su propio pipe y se encarga de copiarlo al directorio `backup/`. Los Workers también actualizan las estadísticas compartidas y envían mensajes al proceso Logger. El Logger registra los eventos de copia en el archivo:

```text
logs/minisync.log
```

## Arquitectura

- **Scanner:** recorre el directorio de manera recursiva y obtiene los metadatos de los archivos.
- **Monitor:** revisa los cambios cada 5 segundos y asigna tareas a los Workers.
- **Workers:** copian los archivos al directorio `backup/`.
- **IPC:** administra los pipes, la memoria compartida y el semáforo.
- **Backup:** realiza la copia con `open()`, `read()`, `write()` y `fsync()`.
- **Logger:** registra los eventos del sistema en un archivo `.log`.
- **Stats:** inicializa y muestra las estadísticas de la sincronización.

La comunicación entre el Monitor y los Workers se realiza mediante pipes.

Cada Worker tiene su propio pipe:

```text
Monitor ---- Pipe 1 ----> Worker 1
Monitor ---- Pipe 2 ----> Worker 2
```

La comunicación entre los Workers y el Logger se realiza mediante un FIFO con nombre.

## Estructura del proyecto

```text
MiniSync/
├── include/
│   ├── file.h
│   ├── scanner.h
│   ├── monitor.h
│   ├── ipc.h
│   ├── stats.h
│   └── logger.h
│
├── src/
│   ├── main.c
│   ├── scanner.c
│   ├── monitor.c
│   ├── backup.c
│   ├── ipc.c
│   ├── stats.c
│   └── logger.c
│
├── originales/
│   ├── archivo1.txt
│   ├── informe.pdf
│   └── documentos/
│       ├── documento.docx
│       └── foto.jpg
│
├── backup/
├── logs/
├── Makefile
└── README.md
```

## Requisitos

El proyecto debe ejecutarse en Linux o WSL.

Se necesita tener instalado:

- GCC
- Make
- Librerías POSIX
- Soporte para procesos, pipes, memoria compartida y semáforos

Para verificar que GCC esté instalado:

```bash
gcc --version
```

Para verificar que Make esté instalado:

```bash
make --version
```

## Compilación

Para compilar el proyecto, se debe abrir una terminal dentro de la carpeta principal y ejecutar:

```bash
make
```

También se puede compilar manualmente con:

```bash
gcc -Wall -Wextra src/*.c -Iinclude -o main -pthread -lrt
```

Después de compilar, se genera un ejecutable llamado:

```text
main
```

Para eliminar el ejecutable:

```bash
make clean
```

## Comandos disponibles

### Mostrar la ayuda

```bash
./main help
```

Este comando muestra los diferentes modos de ejecución del programa.

### Escanear un directorio

```bash
./main scan originales
```

Este comando recorre el directorio una sola vez y muestra los metadatos de los archivos.
Los datos mostrados son:

- Nombre
- Ruta
- Número de i-nodo
- Tamaño
- Permisos
- Fecha de modificación

También se puede ejecutar con:

```bash
make scan
```

### Iniciar el Monitor

```bash
./main monitor originales
```

Este comando inicia el sistema de sincronización en primer plano.
El Monitor revisa el directorio cada 5 segundos, detecta cambios y reparte los archivos entre los dos Workers.
Para detener el Monitor se utiliza:

```text
Ctrl+C
```

También se puede ejecutar con:

```bash
make monitor
```

### Iniciar como daemon

```bash
./main daemon originales
```

Este comando inicia MiniFileSync como un proceso daemon.
En este modo el programa continúa funcionando en segundo plano aunque la terminal quede disponible.
También se puede ejecutar con:

```bash
make daemon
```

### Detener el daemon

```bash
./main stop
```

Este comando lee el PID guardado por el daemon y le envía una señal para detenerlo correctamente.
También se puede utilizar:

```bash
make stop
```

## Prueba del programa

Primero se debe iniciar el Monitor:

```bash
./main monitor originales
```

Luego se abre otra terminal dentro de la carpeta del proyecto.
Para modificar un archivo de texto sin borrar su contenido:

```bash
echo "Cambio de prueba" >> originales/archivo1.txt
```

Para actualizar la fecha de modificación del archivo PDF sin alterar su contenido:

```bash
touch originales/documentos/informe.pdf
```

Después de aproximadamente 5 segundos, el Monitor debería detectar los cambios.
La salida puede mostrar algo similar a:

```text
[MONITOR] Archivo modificado: originales/archivo1.txt
[MONITOR] Archivo modificado: originales/documentos/informe.pdf

[Monitor] originales/archivo1.txt asignado al Worker 1
[Monitor] originales/documentos/informe.pdf asignado al Worker 2
```

Los Workers copiarán los archivos al directorio:

```text
backup/
```

Para comprobar las copias:

```bash
ls -l backup
```

Para revisar el contenido del archivo de texto copiado:

```bash
cat backup/archivo1.txt
```

## Logger

El Logger es un proceso independiente.
Los Workers envían mensajes al Logger mediante un FIFO con nombre.
El Logger agrega la fecha y hora y guarda los eventos en:

```text
logs/minisync.log
```

Para revisar el archivo de registro:

```bash
cat logs/minisync.log
```

El contenido puede verse de esta manera:

```text
[2026-07-17 10:30:15] Worker 1 copio backup/archivo1.txt
[2026-07-17 10:30:16] Worker 2 copio backup/informe.pdf
```

## Estadísticas

Las estadísticas se guardan en memoria compartida mediante:

- `shm_open()`
- `ftruncate()`
- `mmap()`

La estructura utilizada es:

```c
typedef struct stats{

    long archivos_copiados;
    long bytes_copiados;
    long errores;

} Stats;
```

El programa muestra:
- Cantidad de archivos copiados
- Cantidad total de bytes copiados
- Cantidad de errores
Como los dos Workers pueden modificar las estadísticas al mismo tiempo, se utiliza un semáforo POSIX.
Las funciones utilizadas son:

- `sem_open()`
- `sem_wait()`
- `sem_post()`
- `sem_close()`

El semáforo evita que ambos Workers actualicen los datos compartidos al mismo tiempo.

## Sincronización incremental

El programa no copia todos los archivos en cada escaneo.
Un archivo se copia solamente cuando:
- Es un archivo nuevo.
- Cambió su tamaño.
- Cambió su fecha de modificación.
Para detectar los cambios, el Monitor compara los metadatos del escaneo actual con los datos del escaneo anterior.

## Copia de archivos

La función `copiarArchivo()` realiza la copia usando llamadas al sistema de Linux.
Las funciones utilizadas son:

- `open()`
- `read()`
- `write()`
- `fsync()`
- `close()`

No se utilizan comandos externos como:

```text
cp
rsync
system()
```

## Workers

El Monitor crea dos procesos Workers mediante `fork()`.
Cada Worker recibe las tareas por medio de su propio pipe.
La asignación de archivos se realiza alternadamente:

```text
Archivo 1 -> Worker 1
Archivo 2 -> Worker 2
Archivo 3 -> Worker 1
Archivo 4 -> Worker 2
```

De esta forma los dos Workers pueden copiar archivos al mismo tiempo.

## Daemon

El modo daemon permite que MiniFileSync se ejecute en segundo plano.

Para crear el daemon se utilizan:

- `fork()`
- `setsid()`
- `chdir("/")`
- `umask()`

El PID del daemon se guarda en:

```text
/tmp/minifilesync.pid
```

Para consultar el PID:

```bash
cat /tmp/minifilesync.pid
```

Para detenerlo:

```bash
./main stop
```

## Limitaciones

Los archivos se copian directamente dentro del directorio `backup/`.
No se conserva la estructura original de subdirectorios.
Por ejemplo:

```text
originales/documentos/informe.pdf
```

se copia como:

```text
backup/informe.pdf
```

Si existen dos archivos con el mismo nombre en directorios diferentes, uno de ellos podría sobrescribir al otro.
El proyecto fue desarrollado y probado principalmente en Linux mediante WSL.

## Autor

Emilio Albán

Escuela Politécnica Nacional
Ingeniería de Software
Sistemas Operativos

## Repositorio

URL del repositorio:

```text
https://github.com/emilioalbansq/MiniSync
```