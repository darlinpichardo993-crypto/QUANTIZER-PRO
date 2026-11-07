# QUANTIZER PRO — Made by ORYMY · YMY Records

Plugin VST3 que cuantiza instrumentos reales (tocados por humanos) de bachata: segunda guitarra, requinto, adornos, bajo, güira, bongó, conga, tambora, piano.

## Cómo compilarlo en GitHub
1. Crea un repositorio nuevo y sube TODO el contenido de esta carpeta (git add . / commit / push a `main`).
2. Ve a la pestaña **Actions** → "Build Quantizer Pro" (corre solo al hacer push, o con "Run workflow").
3. Al terminar (~10-15 min) descarga en **Artifacts**:
   - `QuantizerPro_Setup` → **QuantizerPro_Setup.exe** (instalador con icono de ojo amarillo)
   - `QuantizerPro_VST3` → carpeta `Quantizer Pro.vst3`
4. Instala, abre FL Studio → Plugin Database → "Find installed plugins".

## Cómo se usa
1. Pon el plugin en una pista. Pon el **TEMPO** de la canción (o activa HOST para usar el del proyecto).
2. Carga el audio: arrástralo sobre el plugin, o botón carpeta. O pulsa **CAPTURAR EN VIVO**, dale play en el DAW y luego DETENER Y ANALIZAR.
   El audio queda alineado a la línea de tiempo del DAW (empieza en el compás 1; ajusta con OFFSET).
3. Elige el **PRESET** del instrumento, el **MOMENTO** y la **REJILLA**. Pulsa **ANALIZAR** y luego **CUANTIZAR**.
   La rejilla muestra las líneas y cada golpe se mueve a su línea. Dale play para escucharlo.
4. MEZCLA compara original vs. cuantizado. **Arrastra el icono de exportar directamente a la Playlist de FL Studio (o a otro DAW)** para insertar el WAV cuantizado sin abrir un diálogo ni guardarlo manualmente. Un clic normal en el icono conserva la exportación tradicional.

El arrastre genera un WAV temporal automáticamente en la carpeta temporal del sistema; se conserva mientras el plugin está abierto y se limpia al cerrar el editor. Así el DAW recibe un archivo WAV estándar y no depende de un formato privado.

## Cómo funciona (sin estirar el audio)
Detecta cada golpe (flujo espectral + refinado en el tiempo), corta el audio en golpes, y MUEVE cada golpe a la rejilla con crossfade corto. No hay time-stretch, así que el timbre no se estira.
