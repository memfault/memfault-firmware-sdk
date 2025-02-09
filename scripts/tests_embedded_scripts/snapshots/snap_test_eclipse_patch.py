#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

# -*- coding: utf-8 -*-
# snapshottest: v1 - https://goo.gl/zC4yUc
from __future__ import unicode_literals

from snapshottest import Snapshot


snapshots = Snapshot()

snapshots['test_eclipse_project_patcher 1'] = {
    '.cproject': '''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
\t<storageModule moduleId="org.eclipse.cdt.core.settings">
\t\t<cconfiguration id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446">
\t\t\t<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" moduleId="org.eclipse.cdt.core.settings" name="Debug">
\t\t\t\t<externalSettings />
\t\t\t\t<extensions>
\t\t\t\t\t<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t</extensions>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t\t\t<configuration artifactExtension="elf" artifactName="STM32L152RE_NUCLEO" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug" cleanCommand="rm -rf" description="" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" name="Debug" parent="fr.ac6.managedbuild.config.gnu.cross.exe.debug" postannouncebuildStep="Generating binary and Printing size information:" postbuildStep="arm-none-eabi-objcopy -O binary &quot;${BuildArtifactFileBaseName}.elf&quot; &quot;${BuildArtifactFileBaseName}.bin&quot; &amp;&amp; arm-none-eabi-size &quot;${BuildArtifactFileName}&quot;">
\t\t\t\t\t<folderInfo id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446." name="/" resourcePath="">
\t\t\t\t\t\t<toolChain id="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug.67646101" name="Ac6 STM32 MCU GCC" superClass="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug">
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.prefix.433022468" name="Prefix" superClass="fr.ac6.managedbuild.option.gnu.cross.prefix" value="arm-none-eabi-" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.mcu.1023045885" name="Mcu" superClass="fr.ac6.managedbuild.option.gnu.cross.mcu" value="STM32L152RETx" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.board.544478532" name="Board" superClass="fr.ac6.managedbuild.option.gnu.cross.board" value="NUCLEO-L152RE" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.instructionSet.1885819322" name="Instruction Set" superClass="fr.ac6.managedbuild.option.gnu.cross.instructionSet" value="fr.ac6.managedbuild.option.gnu.cross.instructionSet.thumbII" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.fpu.460055244" name="Floating point hardware" superClass="fr.ac6.managedbuild.option.gnu.cross.fpu" value="fr.ac6.managedbuild.option.gnu.cross.fpu.no" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.floatabi.1747886298" name="Floating-point ABI" superClass="fr.ac6.managedbuild.option.gnu.cross.floatabi" value="fr.ac6.managedbuild.option.gnu.cross.floatabi.soft" valueType="enumerated" />
\t\t\t\t\t\t\t<targetPlatform archList="all" binaryParser="org.eclipse.cdt.core.ELF" id="fr.ac6.managedbuild.targetPlatform.gnu.cross.66510091" isAbstract="false" osList="all" superClass="fr.ac6.managedbuild.targetPlatform.gnu.cross" />
\t\t\t\t\t\t\t<builder buildPath="${workspace_loc:/STM32L152RE_NUCLEO}/Debug" id="fr.ac6.managedbuild.builder.gnu.cross.583621442" keepEnvironmentInBuildfile="false" managedBuildOn="true" name="Gnu Make Builder" superClass="fr.ac6.managedbuild.builder.gnu.cross">
\t\t\t\t\t\t\t\t<outputEntries>
\t\t\t\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="outputPath" name="Debug" />
\t\t\t\t\t\t\t\t</outputEntries>
\t\t\t\t\t\t\t</builder>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266" name="MCU GCC Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler">
\t\t\t\t\t\t\t\t<option defaultValue="gnu.c.optimization.level.none" id="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level.1209880627" name="Optimization Level" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level" useByScannerDiscovery="false" value="fr.ac6.managedbuild.gnu.c.optimization.level.size" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.debugging.level.589920417" name="Debug Level" superClass="gnu.c.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.c.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.include.paths.182831158" name="Include paths (-I)" superClass="gnu.c.compiler.option.include.paths" useByScannerDiscovery="false" valueType="includePath">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Device/ST/STM32L1xx/Include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/STM32L1xx_HAL_Driver/Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/BSP/STM32L1xx_Nucleo" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Include" />
\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/components/include}&quot;" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/ports/include}&quot;" />
\t\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.preprocessor.def.symbols.824961392" name="Defined symbols (-D)" superClass="gnu.c.compiler.option.preprocessor.def.symbols" useByScannerDiscovery="false" valueType="definedSymbols">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="STM32L152xE" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_STM32L1XX_NUCLEO" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_HAL_DRIVER" />
\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other.867106523" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other" useByScannerDiscovery="false" value="-fmessage-length=0" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s.1769769059" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler.1807716003" name="MCU G++ Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler">
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.optimization.level.555190386" name="Optimization Level" superClass="gnu.cpp.compiler.option.optimization.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.debugging.level.1333063404" name="Debug Level" superClass="gnu.cpp.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.48637223" name="MCU GCC Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker">
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script.2000917657" name="Linker Script (-T)" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script" value="../STM32L152RETx_FLASH.ld" valueType="string" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.libs.493794812" name="Libraries (-l)" superClass="gnu.c.link.option.libs" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.paths.179691201" name="Library search path (-L)" superClass="gnu.c.link.option.paths" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.ldflags.2147304053" name="Linker flags" superClass="gnu.c.link.option.ldflags" value="-specs=nosys.specs -specs=nano.specs" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.c.linker.input.1696379112" superClass="cdt.managedbuild.tool.gnu.c.linker.input">
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)" />
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinput" paths="$(LIBS)" />
\t\t\t\t\t\t\t\t</inputType>
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker.1642466824" name="MCU G++ Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.archiver.401772641" name="MCU GCC Archiver" superClass="fr.ac6.managedbuild.tool.gnu.archiver" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.assembler.756297622" name="MCU GCC Assembler" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler">
\t\t\t\t\t\t\t\t<option id="gnu.both.asm.option.include.paths.1934772483" name="Include paths (-I)" superClass="gnu.both.asm.option.include.paths" valueType="includePath" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1812370298" superClass="cdt.managedbuild.tool.gnu.assembler.input" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.assembler.input.134028949" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler.input" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t</toolChain>
\t\t\t\t\t</folderInfo>
\t\t\t\t\t<sourceEntries>
\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="" />
\t\t\t\t\t</sourceEntries>
\t\t\t\t</configuration>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="org.eclipse.cdt.core.externalSettings" />
\t\t</cconfiguration>
\t</storageModule>
\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t<project id="STM32L152RE_NUCLEO.fr.ac6.managedbuild.target.gnu.cross.exe.1040121120" name="Executable" projectType="fr.ac6.managedbuild.target.gnu.cross.exe" />
\t</storageModule>
\t<storageModule moduleId="scannerConfiguration">
\t\t<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId="" />
\t\t<scannerConfigBuildInfo instanceId="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446;fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446.;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198">
\t\t\t<autodiscovery enabled="false" problemReportingEnabled="true" selectedProfileId="" />
\t\t</scannerConfigBuildInfo>
\t\t
\t</storageModule>
\t<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders" />
\t<storageModule moduleId="refreshScope" versionNumber="2">
\t\t<configuration artifactName="STM32L152RE_NUCLEO" configurationName="Debug">
\t\t\t<resource resourceType="PROJECT" workspacePath="STM32L152RE_NUCLEO" />
\t\t</configuration>
\t</storageModule>
</cproject>''',
    '.project': '''<projectDescription>
\t<name>STM32L152RE_NUCLEO</name>
\t<comment />
\t<projects>
\t</projects>
\t<buildSpec>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
\t\t\t<triggers>clean,full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
\t\t\t<triggers>full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t</buildSpec>
\t<natures>
\t\t<nature>org.eclipse.cdt.core.cnature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
\t\t<nature>fr.ac6.mcu.ide.core.MCUProjectNature</nature>
\t</natures>
\t<linkedResources><link>
\t\t\t<name>Middlewares/FreeRTOS/tasks.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/tasks.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc_ex.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_gpio.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_gpio.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_cortex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_cortex.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/stm32l1xx_it.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/stm32l1xx_it.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/heap_4.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/main.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/main.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/queue.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/queue.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/list.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/list.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr.c</location>
\t\t</link><link>
\t\t\t<name>Application/SW4STM32/startup_stm32l152xe.s</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-1-PROJECT_LOC/startup_stm32l152xe.s</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/port.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr_ex.c</location>
\t\t</link><link>
\t\t\t<name>Doc/readme.txt</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/readme.txt</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/timers.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/timers.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/CMSIS/system_stm32l1xx.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/system_stm32l1xx.c</location>
\t\t</link>
\t\t
\t<link>
\t\t<name>memfault_components</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/arch_arm_cortex_m.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/arch_arm_cortex_m.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_batched_events.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_batched_events.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_build_id.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_build_id.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_compact_log_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_compact_log_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_core_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_core_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_custom_data_recording.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_custom_data_recording.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_export.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_export.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_packetizer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_packetizer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_source_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_source_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_event_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_event_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_heap_stats.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_heap_stats.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log_data_source.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log_data_source.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_ram_reboot_info_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_ram_reboot_info_tracking.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_reboot_tracking_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_reboot_tracking_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_sdk_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_sdk_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_serializer_helper.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_serializer_helper.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_task_watchdog.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_task_watchdog.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_trace_event.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_trace_event.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_base64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_base64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_chunk_transport.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_chunk_transport.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_circular_buffer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_circular_buffer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_crc16_ccitt.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_crc16_ccitt.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_minimal_cbor.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_minimal_cbor.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_varint.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_varint.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_battery.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_battery.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_connectivity.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_connectivity.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_reliability.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_reliability.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_regions_armv7.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_regions_armv7.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_sdk_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_sdk_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_storage_debug.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_storage_debug.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_aarch64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_aarch64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_arm.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_arm.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_armv7_a_r.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_armv7_a_r.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_riscv.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_riscv.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_stdlib_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_stdlib_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/components/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/ports/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/include</locationURI>
\t</link>
\t</linkedResources>
</projectDescription>'''
}

snapshots['test_eclipse_project_patcher_nested_port 1'] = {
    '.cproject': '''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
\t<storageModule moduleId="org.eclipse.cdt.core.settings">
\t\t<cconfiguration id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446">
\t\t\t<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" moduleId="org.eclipse.cdt.core.settings" name="Debug">
\t\t\t\t<externalSettings />
\t\t\t\t<extensions>
\t\t\t\t\t<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t</extensions>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t\t\t<configuration artifactExtension="elf" artifactName="STM32L152RE_NUCLEO" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug" cleanCommand="rm -rf" description="" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" name="Debug" parent="fr.ac6.managedbuild.config.gnu.cross.exe.debug" postannouncebuildStep="Generating binary and Printing size information:" postbuildStep="arm-none-eabi-objcopy -O binary &quot;${BuildArtifactFileBaseName}.elf&quot; &quot;${BuildArtifactFileBaseName}.bin&quot; &amp;&amp; arm-none-eabi-size &quot;${BuildArtifactFileName}&quot;">
\t\t\t\t\t<folderInfo id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446." name="/" resourcePath="">
\t\t\t\t\t\t<toolChain id="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug.67646101" name="Ac6 STM32 MCU GCC" superClass="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug">
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.prefix.433022468" name="Prefix" superClass="fr.ac6.managedbuild.option.gnu.cross.prefix" value="arm-none-eabi-" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.mcu.1023045885" name="Mcu" superClass="fr.ac6.managedbuild.option.gnu.cross.mcu" value="STM32L152RETx" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.board.544478532" name="Board" superClass="fr.ac6.managedbuild.option.gnu.cross.board" value="NUCLEO-L152RE" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.instructionSet.1885819322" name="Instruction Set" superClass="fr.ac6.managedbuild.option.gnu.cross.instructionSet" value="fr.ac6.managedbuild.option.gnu.cross.instructionSet.thumbII" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.fpu.460055244" name="Floating point hardware" superClass="fr.ac6.managedbuild.option.gnu.cross.fpu" value="fr.ac6.managedbuild.option.gnu.cross.fpu.no" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.floatabi.1747886298" name="Floating-point ABI" superClass="fr.ac6.managedbuild.option.gnu.cross.floatabi" value="fr.ac6.managedbuild.option.gnu.cross.floatabi.soft" valueType="enumerated" />
\t\t\t\t\t\t\t<targetPlatform archList="all" binaryParser="org.eclipse.cdt.core.ELF" id="fr.ac6.managedbuild.targetPlatform.gnu.cross.66510091" isAbstract="false" osList="all" superClass="fr.ac6.managedbuild.targetPlatform.gnu.cross" />
\t\t\t\t\t\t\t<builder buildPath="${workspace_loc:/STM32L152RE_NUCLEO}/Debug" id="fr.ac6.managedbuild.builder.gnu.cross.583621442" keepEnvironmentInBuildfile="false" managedBuildOn="true" name="Gnu Make Builder" superClass="fr.ac6.managedbuild.builder.gnu.cross">
\t\t\t\t\t\t\t\t<outputEntries>
\t\t\t\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="outputPath" name="Debug" />
\t\t\t\t\t\t\t\t</outputEntries>
\t\t\t\t\t\t\t</builder>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266" name="MCU GCC Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler">
\t\t\t\t\t\t\t\t<option defaultValue="gnu.c.optimization.level.none" id="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level.1209880627" name="Optimization Level" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level" useByScannerDiscovery="false" value="fr.ac6.managedbuild.gnu.c.optimization.level.size" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.debugging.level.589920417" name="Debug Level" superClass="gnu.c.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.c.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.include.paths.182831158" name="Include paths (-I)" superClass="gnu.c.compiler.option.include.paths" useByScannerDiscovery="false" valueType="includePath">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Device/ST/STM32L1xx/Include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/STM32L1xx_HAL_Driver/Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/BSP/STM32L1xx_Nucleo" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Include" />
\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/components/include}&quot;" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/ports/include}&quot;" />
\t\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.preprocessor.def.symbols.824961392" name="Defined symbols (-D)" superClass="gnu.c.compiler.option.preprocessor.def.symbols" useByScannerDiscovery="false" valueType="definedSymbols">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="STM32L152xE" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_STM32L1XX_NUCLEO" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_HAL_DRIVER" />
\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other.867106523" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other" useByScannerDiscovery="false" value="-fmessage-length=0" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s.1769769059" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler.1807716003" name="MCU G++ Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler">
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.optimization.level.555190386" name="Optimization Level" superClass="gnu.cpp.compiler.option.optimization.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.debugging.level.1333063404" name="Debug Level" superClass="gnu.cpp.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.48637223" name="MCU GCC Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker">
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script.2000917657" name="Linker Script (-T)" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script" value="../STM32L152RETx_FLASH.ld" valueType="string" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.libs.493794812" name="Libraries (-l)" superClass="gnu.c.link.option.libs" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.paths.179691201" name="Library search path (-L)" superClass="gnu.c.link.option.paths" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.ldflags.2147304053" name="Linker flags" superClass="gnu.c.link.option.ldflags" value="-specs=nosys.specs -specs=nano.specs" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.c.linker.input.1696379112" superClass="cdt.managedbuild.tool.gnu.c.linker.input">
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)" />
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinput" paths="$(LIBS)" />
\t\t\t\t\t\t\t\t</inputType>
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker.1642466824" name="MCU G++ Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.archiver.401772641" name="MCU GCC Archiver" superClass="fr.ac6.managedbuild.tool.gnu.archiver" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.assembler.756297622" name="MCU GCC Assembler" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler">
\t\t\t\t\t\t\t\t<option id="gnu.both.asm.option.include.paths.1934772483" name="Include paths (-I)" superClass="gnu.both.asm.option.include.paths" valueType="includePath" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1812370298" superClass="cdt.managedbuild.tool.gnu.assembler.input" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.assembler.input.134028949" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler.input" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t</toolChain>
\t\t\t\t\t</folderInfo>
\t\t\t\t\t<sourceEntries>
\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="" />
\t\t\t\t\t</sourceEntries>
\t\t\t\t</configuration>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="org.eclipse.cdt.core.externalSettings" />
\t\t</cconfiguration>
\t</storageModule>
\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t<project id="STM32L152RE_NUCLEO.fr.ac6.managedbuild.target.gnu.cross.exe.1040121120" name="Executable" projectType="fr.ac6.managedbuild.target.gnu.cross.exe" />
\t</storageModule>
\t<storageModule moduleId="scannerConfiguration">
\t\t<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId="" />
\t\t<scannerConfigBuildInfo instanceId="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446;fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446.;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198">
\t\t\t<autodiscovery enabled="false" problemReportingEnabled="true" selectedProfileId="" />
\t\t</scannerConfigBuildInfo>
\t\t
\t</storageModule>
\t<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders" />
\t<storageModule moduleId="refreshScope" versionNumber="2">
\t\t<configuration artifactName="STM32L152RE_NUCLEO" configurationName="Debug">
\t\t\t<resource resourceType="PROJECT" workspacePath="STM32L152RE_NUCLEO" />
\t\t</configuration>
\t</storageModule>
</cproject>''',
    '.project': '''<projectDescription>
\t<name>STM32L152RE_NUCLEO</name>
\t<comment />
\t<projects>
\t</projects>
\t<buildSpec>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
\t\t\t<triggers>clean,full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
\t\t\t<triggers>full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t</buildSpec>
\t<natures>
\t\t<nature>org.eclipse.cdt.core.cnature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
\t\t<nature>fr.ac6.mcu.ide.core.MCUProjectNature</nature>
\t</natures>
\t<linkedResources><link>
\t\t\t<name>Middlewares/FreeRTOS/tasks.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/tasks.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc_ex.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_gpio.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_gpio.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_cortex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_cortex.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/stm32l1xx_it.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/stm32l1xx_it.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/heap_4.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/main.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/main.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/queue.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/queue.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/list.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/list.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr.c</location>
\t\t</link><link>
\t\t\t<name>Application/SW4STM32/startup_stm32l152xe.s</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-1-PROJECT_LOC/startup_stm32l152xe.s</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/port.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr_ex.c</location>
\t\t</link><link>
\t\t\t<name>Doc/readme.txt</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/readme.txt</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/timers.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/timers.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/CMSIS/system_stm32l1xx.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/system_stm32l1xx.c</location>
\t\t</link>
\t\t
\t<link>
\t\t<name>memfault_components</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/arch_arm_cortex_m.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/arch_arm_cortex_m.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_batched_events.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_batched_events.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_build_id.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_build_id.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_compact_log_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_compact_log_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_core_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_core_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_custom_data_recording.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_custom_data_recording.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_export.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_export.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_packetizer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_packetizer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_source_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_source_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_event_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_event_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_heap_stats.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_heap_stats.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log_data_source.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log_data_source.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_ram_reboot_info_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_ram_reboot_info_tracking.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_reboot_tracking_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_reboot_tracking_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_sdk_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_sdk_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_serializer_helper.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_serializer_helper.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_task_watchdog.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_task_watchdog.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_trace_event.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_trace_event.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_base64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_base64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_chunk_transport.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_chunk_transport.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_circular_buffer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_circular_buffer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_crc16_ccitt.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_crc16_ccitt.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_minimal_cbor.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_minimal_cbor.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_varint.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_varint.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_battery.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_battery.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_connectivity.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_connectivity.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_reliability.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_reliability.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_regions_armv7.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_regions_armv7.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_sdk_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_sdk_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_storage_debug.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_storage_debug.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_aarch64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_aarch64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_arm.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_arm.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_armv7_a_r.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_armv7_a_r.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_riscv.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_riscv.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_stdlib_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_stdlib_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/components/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/ports/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_core.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_core.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_coredump_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_coredump_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_coredump_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_coredump_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_debug_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_debug_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/reset_reboot_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/reset_reboot_tracking.c</locationURI>
\t</link>
\t</linkedResources>
</projectDescription>'''
}

snapshots['test_eclipse_project_patcher_single_dir_port 1'] = {
    '.cproject': '''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
\t<storageModule moduleId="org.eclipse.cdt.core.settings">
\t\t<cconfiguration id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446">
\t\t\t<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" moduleId="org.eclipse.cdt.core.settings" name="Debug">
\t\t\t\t<externalSettings />
\t\t\t\t<extensions>
\t\t\t\t\t<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t</extensions>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t\t\t<configuration artifactExtension="elf" artifactName="STM32L152RE_NUCLEO" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug" cleanCommand="rm -rf" description="" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" name="Debug" parent="fr.ac6.managedbuild.config.gnu.cross.exe.debug" postannouncebuildStep="Generating binary and Printing size information:" postbuildStep="arm-none-eabi-objcopy -O binary &quot;${BuildArtifactFileBaseName}.elf&quot; &quot;${BuildArtifactFileBaseName}.bin&quot; &amp;&amp; arm-none-eabi-size &quot;${BuildArtifactFileName}&quot;">
\t\t\t\t\t<folderInfo id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446." name="/" resourcePath="">
\t\t\t\t\t\t<toolChain id="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug.67646101" name="Ac6 STM32 MCU GCC" superClass="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug">
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.prefix.433022468" name="Prefix" superClass="fr.ac6.managedbuild.option.gnu.cross.prefix" value="arm-none-eabi-" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.mcu.1023045885" name="Mcu" superClass="fr.ac6.managedbuild.option.gnu.cross.mcu" value="STM32L152RETx" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.board.544478532" name="Board" superClass="fr.ac6.managedbuild.option.gnu.cross.board" value="NUCLEO-L152RE" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.instructionSet.1885819322" name="Instruction Set" superClass="fr.ac6.managedbuild.option.gnu.cross.instructionSet" value="fr.ac6.managedbuild.option.gnu.cross.instructionSet.thumbII" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.fpu.460055244" name="Floating point hardware" superClass="fr.ac6.managedbuild.option.gnu.cross.fpu" value="fr.ac6.managedbuild.option.gnu.cross.fpu.no" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.floatabi.1747886298" name="Floating-point ABI" superClass="fr.ac6.managedbuild.option.gnu.cross.floatabi" value="fr.ac6.managedbuild.option.gnu.cross.floatabi.soft" valueType="enumerated" />
\t\t\t\t\t\t\t<targetPlatform archList="all" binaryParser="org.eclipse.cdt.core.ELF" id="fr.ac6.managedbuild.targetPlatform.gnu.cross.66510091" isAbstract="false" osList="all" superClass="fr.ac6.managedbuild.targetPlatform.gnu.cross" />
\t\t\t\t\t\t\t<builder buildPath="${workspace_loc:/STM32L152RE_NUCLEO}/Debug" id="fr.ac6.managedbuild.builder.gnu.cross.583621442" keepEnvironmentInBuildfile="false" managedBuildOn="true" name="Gnu Make Builder" superClass="fr.ac6.managedbuild.builder.gnu.cross">
\t\t\t\t\t\t\t\t<outputEntries>
\t\t\t\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="outputPath" name="Debug" />
\t\t\t\t\t\t\t\t</outputEntries>
\t\t\t\t\t\t\t</builder>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266" name="MCU GCC Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler">
\t\t\t\t\t\t\t\t<option defaultValue="gnu.c.optimization.level.none" id="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level.1209880627" name="Optimization Level" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level" useByScannerDiscovery="false" value="fr.ac6.managedbuild.gnu.c.optimization.level.size" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.debugging.level.589920417" name="Debug Level" superClass="gnu.c.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.c.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.include.paths.182831158" name="Include paths (-I)" superClass="gnu.c.compiler.option.include.paths" useByScannerDiscovery="false" valueType="includePath">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Device/ST/STM32L1xx/Include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/STM32L1xx_HAL_Driver/Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/BSP/STM32L1xx_Nucleo" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Include" />
\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/components/include}&quot;" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/ports/include}&quot;" />
\t\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.preprocessor.def.symbols.824961392" name="Defined symbols (-D)" superClass="gnu.c.compiler.option.preprocessor.def.symbols" useByScannerDiscovery="false" valueType="definedSymbols">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="STM32L152xE" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_STM32L1XX_NUCLEO" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_HAL_DRIVER" />
\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other.867106523" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other" useByScannerDiscovery="false" value="-fmessage-length=0" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s.1769769059" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler.1807716003" name="MCU G++ Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler">
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.optimization.level.555190386" name="Optimization Level" superClass="gnu.cpp.compiler.option.optimization.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.debugging.level.1333063404" name="Debug Level" superClass="gnu.cpp.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.48637223" name="MCU GCC Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker">
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script.2000917657" name="Linker Script (-T)" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script" value="../STM32L152RETx_FLASH.ld" valueType="string" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.libs.493794812" name="Libraries (-l)" superClass="gnu.c.link.option.libs" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.paths.179691201" name="Library search path (-L)" superClass="gnu.c.link.option.paths" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.ldflags.2147304053" name="Linker flags" superClass="gnu.c.link.option.ldflags" value="-specs=nosys.specs -specs=nano.specs" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.c.linker.input.1696379112" superClass="cdt.managedbuild.tool.gnu.c.linker.input">
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)" />
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinput" paths="$(LIBS)" />
\t\t\t\t\t\t\t\t</inputType>
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker.1642466824" name="MCU G++ Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.archiver.401772641" name="MCU GCC Archiver" superClass="fr.ac6.managedbuild.tool.gnu.archiver" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.assembler.756297622" name="MCU GCC Assembler" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler">
\t\t\t\t\t\t\t\t<option id="gnu.both.asm.option.include.paths.1934772483" name="Include paths (-I)" superClass="gnu.both.asm.option.include.paths" valueType="includePath" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1812370298" superClass="cdt.managedbuild.tool.gnu.assembler.input" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.assembler.input.134028949" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler.input" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t</toolChain>
\t\t\t\t\t</folderInfo>
\t\t\t\t\t<sourceEntries>
\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="" />
\t\t\t\t\t</sourceEntries>
\t\t\t\t</configuration>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="org.eclipse.cdt.core.externalSettings" />
\t\t</cconfiguration>
\t</storageModule>
\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t<project id="STM32L152RE_NUCLEO.fr.ac6.managedbuild.target.gnu.cross.exe.1040121120" name="Executable" projectType="fr.ac6.managedbuild.target.gnu.cross.exe" />
\t</storageModule>
\t<storageModule moduleId="scannerConfiguration">
\t\t<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId="" />
\t\t<scannerConfigBuildInfo instanceId="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446;fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446.;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198">
\t\t\t<autodiscovery enabled="false" problemReportingEnabled="true" selectedProfileId="" />
\t\t</scannerConfigBuildInfo>
\t\t
\t</storageModule>
\t<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders" />
\t<storageModule moduleId="refreshScope" versionNumber="2">
\t\t<configuration artifactName="STM32L152RE_NUCLEO" configurationName="Debug">
\t\t\t<resource resourceType="PROJECT" workspacePath="STM32L152RE_NUCLEO" />
\t\t</configuration>
\t</storageModule>
</cproject>''',
    '.project': '''<projectDescription>
\t<name>STM32L152RE_NUCLEO</name>
\t<comment />
\t<projects>
\t</projects>
\t<buildSpec>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
\t\t\t<triggers>clean,full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
\t\t\t<triggers>full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t</buildSpec>
\t<natures>
\t\t<nature>org.eclipse.cdt.core.cnature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
\t\t<nature>fr.ac6.mcu.ide.core.MCUProjectNature</nature>
\t</natures>
\t<linkedResources><link>
\t\t\t<name>Middlewares/FreeRTOS/tasks.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/tasks.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc_ex.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_gpio.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_gpio.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_cortex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_cortex.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/stm32l1xx_it.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/stm32l1xx_it.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/heap_4.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/main.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/main.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/queue.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/queue.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/list.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/list.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr.c</location>
\t\t</link><link>
\t\t\t<name>Application/SW4STM32/startup_stm32l152xe.s</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-1-PROJECT_LOC/startup_stm32l152xe.s</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/port.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr_ex.c</location>
\t\t</link><link>
\t\t\t<name>Doc/readme.txt</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/readme.txt</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/timers.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/timers.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/CMSIS/system_stm32l1xx.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/system_stm32l1xx.c</location>
\t\t</link>
\t\t
\t<link>
\t\t<name>memfault_components</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/arch_arm_cortex_m.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/arch_arm_cortex_m.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_batched_events.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_batched_events.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_build_id.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_build_id.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_compact_log_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_compact_log_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_core_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_core_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_custom_data_recording.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_custom_data_recording.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_export.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_export.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_packetizer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_packetizer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_source_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_source_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_event_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_event_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_heap_stats.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_heap_stats.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log_data_source.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log_data_source.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_ram_reboot_info_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_ram_reboot_info_tracking.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_reboot_tracking_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_reboot_tracking_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_sdk_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_sdk_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_serializer_helper.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_serializer_helper.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_task_watchdog.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_task_watchdog.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_trace_event.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_trace_event.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_base64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_base64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_chunk_transport.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_chunk_transport.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_circular_buffer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_circular_buffer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_crc16_ccitt.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_crc16_ccitt.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_minimal_cbor.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_minimal_cbor.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_varint.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_varint.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_battery.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_battery.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_connectivity.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_connectivity.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_reliability.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_reliability.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_regions_armv7.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_regions_armv7.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_sdk_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_sdk_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_storage_debug.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_storage_debug.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_aarch64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_aarch64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_arm.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_arm.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_armv7_a_r.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_armv7_a_r.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_riscv.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_riscv.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_stdlib_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_stdlib_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/components/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/ports/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_core_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_core_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_freertos_ram_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_freertos_ram_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_metrics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_metrics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_panics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_panics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_sdk_metrics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_sdk_metrics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_sdk_metrics_thread.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_sdk_metrics_thread.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_self_test_platform.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_self_test_platform.c</locationURI>
\t</link>
\t</linkedResources>
</projectDescription>'''
}

snapshots['test_eclipse_project_patcher_nested_port 1'] = {
    '.cproject': '''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
\t<storageModule moduleId="org.eclipse.cdt.core.settings">
\t\t<cconfiguration id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446">
\t\t\t<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" moduleId="org.eclipse.cdt.core.settings" name="Debug">
\t\t\t\t<externalSettings />
\t\t\t\t<extensions>
\t\t\t\t\t<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t</extensions>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t\t\t<configuration artifactExtension="elf" artifactName="STM32L152RE_NUCLEO" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug" cleanCommand="rm -rf" description="" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" name="Debug" parent="fr.ac6.managedbuild.config.gnu.cross.exe.debug" postannouncebuildStep="Generating binary and Printing size information:" postbuildStep="arm-none-eabi-objcopy -O binary &quot;${BuildArtifactFileBaseName}.elf&quot; &quot;${BuildArtifactFileBaseName}.bin&quot; &amp;&amp; arm-none-eabi-size &quot;${BuildArtifactFileName}&quot;">
\t\t\t\t\t<folderInfo id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446." name="/" resourcePath="">
\t\t\t\t\t\t<toolChain id="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug.67646101" name="Ac6 STM32 MCU GCC" superClass="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug">
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.prefix.433022468" name="Prefix" superClass="fr.ac6.managedbuild.option.gnu.cross.prefix" value="arm-none-eabi-" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.mcu.1023045885" name="Mcu" superClass="fr.ac6.managedbuild.option.gnu.cross.mcu" value="STM32L152RETx" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.board.544478532" name="Board" superClass="fr.ac6.managedbuild.option.gnu.cross.board" value="NUCLEO-L152RE" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.instructionSet.1885819322" name="Instruction Set" superClass="fr.ac6.managedbuild.option.gnu.cross.instructionSet" value="fr.ac6.managedbuild.option.gnu.cross.instructionSet.thumbII" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.fpu.460055244" name="Floating point hardware" superClass="fr.ac6.managedbuild.option.gnu.cross.fpu" value="fr.ac6.managedbuild.option.gnu.cross.fpu.no" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.floatabi.1747886298" name="Floating-point ABI" superClass="fr.ac6.managedbuild.option.gnu.cross.floatabi" value="fr.ac6.managedbuild.option.gnu.cross.floatabi.soft" valueType="enumerated" />
\t\t\t\t\t\t\t<targetPlatform archList="all" binaryParser="org.eclipse.cdt.core.ELF" id="fr.ac6.managedbuild.targetPlatform.gnu.cross.66510091" isAbstract="false" osList="all" superClass="fr.ac6.managedbuild.targetPlatform.gnu.cross" />
\t\t\t\t\t\t\t<builder buildPath="${workspace_loc:/STM32L152RE_NUCLEO}/Debug" id="fr.ac6.managedbuild.builder.gnu.cross.583621442" keepEnvironmentInBuildfile="false" managedBuildOn="true" name="Gnu Make Builder" superClass="fr.ac6.managedbuild.builder.gnu.cross">
\t\t\t\t\t\t\t\t<outputEntries>
\t\t\t\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="outputPath" name="Debug" />
\t\t\t\t\t\t\t\t</outputEntries>
\t\t\t\t\t\t\t</builder>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266" name="MCU GCC Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler">
\t\t\t\t\t\t\t\t<option defaultValue="gnu.c.optimization.level.none" id="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level.1209880627" name="Optimization Level" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level" useByScannerDiscovery="false" value="fr.ac6.managedbuild.gnu.c.optimization.level.size" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.debugging.level.589920417" name="Debug Level" superClass="gnu.c.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.c.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.include.paths.182831158" name="Include paths (-I)" superClass="gnu.c.compiler.option.include.paths" useByScannerDiscovery="false" valueType="includePath">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Device/ST/STM32L1xx/Include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/STM32L1xx_HAL_Driver/Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/BSP/STM32L1xx_Nucleo" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Include" />
\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/components/include}&quot;" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/ports/include}&quot;" />
\t\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.preprocessor.def.symbols.824961392" name="Defined symbols (-D)" superClass="gnu.c.compiler.option.preprocessor.def.symbols" useByScannerDiscovery="false" valueType="definedSymbols">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="STM32L152xE" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_STM32L1XX_NUCLEO" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_HAL_DRIVER" />
\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other.867106523" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other" useByScannerDiscovery="false" value="-fmessage-length=0" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s.1769769059" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler.1807716003" name="MCU G++ Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler">
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.optimization.level.555190386" name="Optimization Level" superClass="gnu.cpp.compiler.option.optimization.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.debugging.level.1333063404" name="Debug Level" superClass="gnu.cpp.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.48637223" name="MCU GCC Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker">
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script.2000917657" name="Linker Script (-T)" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script" value="../STM32L152RETx_FLASH.ld" valueType="string" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.libs.493794812" name="Libraries (-l)" superClass="gnu.c.link.option.libs" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.paths.179691201" name="Library search path (-L)" superClass="gnu.c.link.option.paths" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.ldflags.2147304053" name="Linker flags" superClass="gnu.c.link.option.ldflags" value="-specs=nosys.specs -specs=nano.specs" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.c.linker.input.1696379112" superClass="cdt.managedbuild.tool.gnu.c.linker.input">
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)" />
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinput" paths="$(LIBS)" />
\t\t\t\t\t\t\t\t</inputType>
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker.1642466824" name="MCU G++ Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.archiver.401772641" name="MCU GCC Archiver" superClass="fr.ac6.managedbuild.tool.gnu.archiver" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.assembler.756297622" name="MCU GCC Assembler" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler">
\t\t\t\t\t\t\t\t<option id="gnu.both.asm.option.include.paths.1934772483" name="Include paths (-I)" superClass="gnu.both.asm.option.include.paths" valueType="includePath" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1812370298" superClass="cdt.managedbuild.tool.gnu.assembler.input" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.assembler.input.134028949" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler.input" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t</toolChain>
\t\t\t\t\t</folderInfo>
\t\t\t\t\t<sourceEntries>
\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="" />
\t\t\t\t\t</sourceEntries>
\t\t\t\t</configuration>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="org.eclipse.cdt.core.externalSettings" />
\t\t</cconfiguration>
\t</storageModule>
\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t<project id="STM32L152RE_NUCLEO.fr.ac6.managedbuild.target.gnu.cross.exe.1040121120" name="Executable" projectType="fr.ac6.managedbuild.target.gnu.cross.exe" />
\t</storageModule>
\t<storageModule moduleId="scannerConfiguration">
\t\t<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId="" />
\t\t<scannerConfigBuildInfo instanceId="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446;fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446.;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198">
\t\t\t<autodiscovery enabled="false" problemReportingEnabled="true" selectedProfileId="" />
\t\t</scannerConfigBuildInfo>
\t\t
\t</storageModule>
\t<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders" />
\t<storageModule moduleId="refreshScope" versionNumber="2">
\t\t<configuration artifactName="STM32L152RE_NUCLEO" configurationName="Debug">
\t\t\t<resource resourceType="PROJECT" workspacePath="STM32L152RE_NUCLEO" />
\t\t</configuration>
\t</storageModule>
</cproject>''',
    '.project': '''<projectDescription>
\t<name>STM32L152RE_NUCLEO</name>
\t<comment />
\t<projects>
\t</projects>
\t<buildSpec>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
\t\t\t<triggers>clean,full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
\t\t\t<triggers>full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t</buildSpec>
\t<natures>
\t\t<nature>org.eclipse.cdt.core.cnature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
\t\t<nature>fr.ac6.mcu.ide.core.MCUProjectNature</nature>
\t</natures>
\t<linkedResources><link>
\t\t\t<name>Middlewares/FreeRTOS/tasks.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/tasks.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc_ex.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_gpio.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_gpio.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_cortex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_cortex.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/stm32l1xx_it.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/stm32l1xx_it.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/heap_4.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/main.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/main.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/queue.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/queue.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/list.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/list.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr.c</location>
\t\t</link><link>
\t\t\t<name>Application/SW4STM32/startup_stm32l152xe.s</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-1-PROJECT_LOC/startup_stm32l152xe.s</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/port.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr_ex.c</location>
\t\t</link><link>
\t\t\t<name>Doc/readme.txt</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/readme.txt</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/timers.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/timers.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/CMSIS/system_stm32l1xx.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/system_stm32l1xx.c</location>
\t\t</link>
\t\t
\t<link>
\t\t<name>memfault_components</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/arch_arm_cortex_m.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/arch_arm_cortex_m.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_batched_events.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_batched_events.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_build_id.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_build_id.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_compact_log_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_compact_log_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_core_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_core_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_custom_data_recording.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_custom_data_recording.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_export.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_export.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_packetizer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_packetizer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_source_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_source_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_event_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_event_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_heap_stats.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_heap_stats.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log_data_source.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log_data_source.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_ram_reboot_info_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_ram_reboot_info_tracking.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_reboot_tracking_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_reboot_tracking_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_sdk_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_sdk_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_serializer_helper.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_serializer_helper.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_task_watchdog.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_task_watchdog.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_trace_event.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_trace_event.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_base64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_base64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_chunk_transport.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_chunk_transport.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_circular_buffer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_circular_buffer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_crc16_ccitt.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_crc16_ccitt.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_minimal_cbor.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_minimal_cbor.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_varint.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_varint.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_battery.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_battery.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_connectivity.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_connectivity.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_reliability.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_reliability.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_regions_armv7.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_regions_armv7.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_sdk_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_sdk_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_storage_debug.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_storage_debug.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_aarch64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_aarch64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_arm.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_arm.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_armv7_a_r.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_armv7_a_r.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_riscv.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_riscv.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_stdlib_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_stdlib_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/components/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/ports/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_core.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_core.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_coredump_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_coredump_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_coredump_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_coredump_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_debug_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_debug_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/memfault_platform_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/memfault_platform_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_dialog_da145xx/reset_reboot_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/dialog/da145xx/reset_reboot_tracking.c</locationURI>
\t</link>
\t</linkedResources>
</projectDescription>'''
}

snapshots['test_eclipse_project_patcher_single_dir_port 1'] = {
    '.cproject': '''<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<?fileVersion 4.0.0?><cproject storage_type_id="org.eclipse.cdt.core.XmlProjectDescriptionStorage">
\t<storageModule moduleId="org.eclipse.cdt.core.settings">
\t\t<cconfiguration id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446">
\t\t\t<storageModule buildSystemId="org.eclipse.cdt.managedbuilder.core.configurationDataProvider" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" moduleId="org.eclipse.cdt.core.settings" name="Debug">
\t\t\t\t<externalSettings />
\t\t\t\t<extensions>
\t\t\t\t\t<extension id="org.eclipse.cdt.core.ELF" point="org.eclipse.cdt.core.BinaryParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GASErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GmakeErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GLDErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.CWDLocator" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t\t<extension id="org.eclipse.cdt.core.GCCErrorParser" point="org.eclipse.cdt.core.ErrorParser" />
\t\t\t\t</extensions>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t\t\t<configuration artifactExtension="elf" artifactName="STM32L152RE_NUCLEO" buildArtefactType="org.eclipse.cdt.build.core.buildArtefactType.exe" buildProperties="org.eclipse.cdt.build.core.buildArtefactType=org.eclipse.cdt.build.core.buildArtefactType.exe,org.eclipse.cdt.build.core.buildType=org.eclipse.cdt.build.core.buildType.debug" cleanCommand="rm -rf" description="" id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446" name="Debug" parent="fr.ac6.managedbuild.config.gnu.cross.exe.debug" postannouncebuildStep="Generating binary and Printing size information:" postbuildStep="arm-none-eabi-objcopy -O binary &quot;${BuildArtifactFileBaseName}.elf&quot; &quot;${BuildArtifactFileBaseName}.bin&quot; &amp;&amp; arm-none-eabi-size &quot;${BuildArtifactFileName}&quot;">
\t\t\t\t\t<folderInfo id="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446." name="/" resourcePath="">
\t\t\t\t\t\t<toolChain id="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug.67646101" name="Ac6 STM32 MCU GCC" superClass="fr.ac6.managedbuild.toolchain.gnu.cross.exe.debug">
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.prefix.433022468" name="Prefix" superClass="fr.ac6.managedbuild.option.gnu.cross.prefix" value="arm-none-eabi-" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.mcu.1023045885" name="Mcu" superClass="fr.ac6.managedbuild.option.gnu.cross.mcu" value="STM32L152RETx" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.board.544478532" name="Board" superClass="fr.ac6.managedbuild.option.gnu.cross.board" value="NUCLEO-L152RE" valueType="string" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.instructionSet.1885819322" name="Instruction Set" superClass="fr.ac6.managedbuild.option.gnu.cross.instructionSet" value="fr.ac6.managedbuild.option.gnu.cross.instructionSet.thumbII" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.fpu.460055244" name="Floating point hardware" superClass="fr.ac6.managedbuild.option.gnu.cross.fpu" value="fr.ac6.managedbuild.option.gnu.cross.fpu.no" valueType="enumerated" />
\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.option.gnu.cross.floatabi.1747886298" name="Floating-point ABI" superClass="fr.ac6.managedbuild.option.gnu.cross.floatabi" value="fr.ac6.managedbuild.option.gnu.cross.floatabi.soft" valueType="enumerated" />
\t\t\t\t\t\t\t<targetPlatform archList="all" binaryParser="org.eclipse.cdt.core.ELF" id="fr.ac6.managedbuild.targetPlatform.gnu.cross.66510091" isAbstract="false" osList="all" superClass="fr.ac6.managedbuild.targetPlatform.gnu.cross" />
\t\t\t\t\t\t\t<builder buildPath="${workspace_loc:/STM32L152RE_NUCLEO}/Debug" id="fr.ac6.managedbuild.builder.gnu.cross.583621442" keepEnvironmentInBuildfile="false" managedBuildOn="true" name="Gnu Make Builder" superClass="fr.ac6.managedbuild.builder.gnu.cross">
\t\t\t\t\t\t\t\t<outputEntries>
\t\t\t\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="outputPath" name="Debug" />
\t\t\t\t\t\t\t\t</outputEntries>
\t\t\t\t\t\t\t</builder>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266" name="MCU GCC Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler">
\t\t\t\t\t\t\t\t<option defaultValue="gnu.c.optimization.level.none" id="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level.1209880627" name="Optimization Level" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.optimization.level" useByScannerDiscovery="false" value="fr.ac6.managedbuild.gnu.c.optimization.level.size" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.debugging.level.589920417" name="Debug Level" superClass="gnu.c.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.c.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.include.paths.182831158" name="Include paths (-I)" superClass="gnu.c.compiler.option.include.paths" useByScannerDiscovery="false" valueType="includePath">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Device/ST/STM32L1xx/Include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/STM32L1xx_HAL_Driver/Inc" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/BSP/STM32L1xx_Nucleo" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Middlewares/Third_Party/FreeRTOS/Source/include" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="../../../../../../../../Drivers/CMSIS/Include" />
\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/components/include}&quot;" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtin="false" value="&quot;${workspace_loc:/${ProjName}/memfault_includes/ports/include}&quot;" />
\t\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="gnu.c.compiler.option.preprocessor.def.symbols.824961392" name="Defined symbols (-D)" superClass="gnu.c.compiler.option.preprocessor.def.symbols" useByScannerDiscovery="false" valueType="definedSymbols">
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="STM32L152xE" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_STM32L1XX_NUCLEO" />
\t\t\t\t\t\t\t\t\t<listOptionValue builtIn="false" value="USE_HAL_DRIVER" />
\t\t\t\t\t\t\t\t</option>
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other.867106523" superClass="fr.ac6.managedbuild.gnu.c.compiler.option.misc.other" useByScannerDiscovery="false" value="-fmessage-length=0" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s.1769769059" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.s" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler.1807716003" name="MCU G++ Compiler" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.compiler">
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.optimization.level.555190386" name="Optimization Level" superClass="gnu.cpp.compiler.option.optimization.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.optimization.level.none" valueType="enumerated" />
\t\t\t\t\t\t\t\t<option id="gnu.cpp.compiler.option.debugging.level.1333063404" name="Debug Level" superClass="gnu.cpp.compiler.option.debugging.level" useByScannerDiscovery="false" value="gnu.cpp.compiler.debugging.level.max" valueType="enumerated" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.48637223" name="MCU GCC Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker">
\t\t\t\t\t\t\t\t<option id="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script.2000917657" name="Linker Script (-T)" superClass="fr.ac6.managedbuild.tool.gnu.cross.c.linker.script" value="../STM32L152RETx_FLASH.ld" valueType="string" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.libs.493794812" name="Libraries (-l)" superClass="gnu.c.link.option.libs" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.paths.179691201" name="Library search path (-L)" superClass="gnu.c.link.option.paths" />
\t\t\t\t\t\t\t\t<option id="gnu.c.link.option.ldflags.2147304053" name="Linker flags" superClass="gnu.c.link.option.ldflags" value="-specs=nosys.specs -specs=nano.specs" valueType="string" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.c.linker.input.1696379112" superClass="cdt.managedbuild.tool.gnu.c.linker.input">
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinputdependency" paths="$(USER_OBJS)" />
\t\t\t\t\t\t\t\t\t<additionalInput kind="additionalinput" paths="$(LIBS)" />
\t\t\t\t\t\t\t\t</inputType>
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker.1642466824" name="MCU G++ Linker" superClass="fr.ac6.managedbuild.tool.gnu.cross.cpp.linker" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.archiver.401772641" name="MCU GCC Archiver" superClass="fr.ac6.managedbuild.tool.gnu.archiver" />
\t\t\t\t\t\t\t<tool id="fr.ac6.managedbuild.tool.gnu.cross.assembler.756297622" name="MCU GCC Assembler" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler">
\t\t\t\t\t\t\t\t<option id="gnu.both.asm.option.include.paths.1934772483" name="Include paths (-I)" superClass="gnu.both.asm.option.include.paths" valueType="includePath" />
\t\t\t\t\t\t\t\t<inputType id="cdt.managedbuild.tool.gnu.assembler.input.1812370298" superClass="cdt.managedbuild.tool.gnu.assembler.input" />
\t\t\t\t\t\t\t\t<inputType id="fr.ac6.managedbuild.tool.gnu.cross.assembler.input.134028949" superClass="fr.ac6.managedbuild.tool.gnu.cross.assembler.input" />
\t\t\t\t\t\t\t</tool>
\t\t\t\t\t\t</toolChain>
\t\t\t\t\t</folderInfo>
\t\t\t\t\t<sourceEntries>
\t\t\t\t\t\t<entry flags="VALUE_WORKSPACE_PATH|RESOLVED" kind="sourcePath" name="" />
\t\t\t\t\t</sourceEntries>
\t\t\t\t</configuration>
\t\t\t</storageModule>
\t\t\t<storageModule moduleId="org.eclipse.cdt.core.externalSettings" />
\t\t</cconfiguration>
\t</storageModule>
\t<storageModule moduleId="cdtBuildSystem" version="4.0.0">
\t\t<project id="STM32L152RE_NUCLEO.fr.ac6.managedbuild.target.gnu.cross.exe.1040121120" name="Executable" projectType="fr.ac6.managedbuild.target.gnu.cross.exe" />
\t</storageModule>
\t<storageModule moduleId="scannerConfiguration">
\t\t<autodiscovery enabled="true" problemReportingEnabled="true" selectedProfileId="" />
\t\t<scannerConfigBuildInfo instanceId="fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446;fr.ac6.managedbuild.config.gnu.cross.exe.debug.900604446.;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.531660266;fr.ac6.managedbuild.tool.gnu.cross.c.compiler.input.c.500700198">
\t\t\t<autodiscovery enabled="false" problemReportingEnabled="true" selectedProfileId="" />
\t\t</scannerConfigBuildInfo>
\t\t
\t</storageModule>
\t<storageModule moduleId="org.eclipse.cdt.core.LanguageSettingsProviders" />
\t<storageModule moduleId="refreshScope" versionNumber="2">
\t\t<configuration artifactName="STM32L152RE_NUCLEO" configurationName="Debug">
\t\t\t<resource resourceType="PROJECT" workspacePath="STM32L152RE_NUCLEO" />
\t\t</configuration>
\t</storageModule>
</cproject>''',
    '.project': '''<projectDescription>
\t<name>STM32L152RE_NUCLEO</name>
\t<comment />
\t<projects>
\t</projects>
\t<buildSpec>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.genmakebuilder</name>
\t\t\t<triggers>clean,full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t\t<buildCommand>
\t\t\t<name>org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder</name>
\t\t\t<triggers>full,incremental,</triggers>
\t\t\t<arguments>
\t\t\t</arguments>
\t\t</buildCommand>
\t</buildSpec>
\t<natures>
\t\t<nature>org.eclipse.cdt.core.cnature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.managedBuildNature</nature>
\t\t<nature>org.eclipse.cdt.managedbuilder.core.ScannerConfigNature</nature>
\t\t<nature>fr.ac6.mcu.ide.core.MCUProjectNature</nature>
\t</natures>
\t<linkedResources><link>
\t\t\t<name>Middlewares/FreeRTOS/tasks.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/tasks.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc_ex.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_gpio.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_gpio.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_cortex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_cortex.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/stm32l1xx_it.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/stm32l1xx_it.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/heap_4.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/CMSIS_RTOS/cmsis_os.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/BSP/STM32L1xx_Nucleo/stm32l1xx_nucleo.c</location>
\t\t</link><link>
\t\t\t<name>Application/User/main.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/main.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/queue.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/queue.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/list.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/list.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr.c</location>
\t\t</link><link>
\t\t\t<name>Application/SW4STM32/startup_stm32l152xe.s</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-1-PROJECT_LOC/startup_stm32l152xe.s</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_rcc.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_rcc.c</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/portable/port.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/STM32L1xx_HAL_Driver/stm32l1xx_hal_pwr_ex.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Drivers/STM32L1xx_HAL_Driver/Src/stm32l1xx_hal_pwr_ex.c</location>
\t\t</link><link>
\t\t\t<name>Doc/readme.txt</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/readme.txt</location>
\t\t</link><link>
\t\t\t<name>Middlewares/FreeRTOS/timers.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-7-PROJECT_LOC/Middlewares/Third_Party/FreeRTOS/Source/timers.c</location>
\t\t</link><link>
\t\t\t<name>Drivers/CMSIS/system_stm32l1xx.c</name>
\t\t\t<type>1</type>
\t\t\t<location>PARENT-2-PROJECT_LOC/Src/system_stm32l1xx.c</location>
\t\t</link>
\t\t
\t<link>
\t\t<name>memfault_components</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/arch_arm_cortex_m.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/arch_arm_cortex_m.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_batched_events.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_batched_events.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_build_id.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_build_id.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_compact_log_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_compact_log_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_core_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_core_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_custom_data_recording.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_custom_data_recording.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_export.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_export.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_packetizer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_packetizer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_data_source_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_data_source_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_event_storage.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_event_storage.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_heap_stats.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_heap_stats.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_log_data_source.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_log_data_source.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_ram_reboot_info_tracking.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_ram_reboot_info_tracking.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_reboot_tracking_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_reboot_tracking_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_sdk_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_sdk_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_self_test_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_self_test_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_serializer_helper.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_serializer_helper.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_task_watchdog.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_task_watchdog.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_trace_event.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/core/src/memfault_trace_event.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_base64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_base64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_chunk_transport.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_chunk_transport.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_circular_buffer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_circular_buffer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_crc16_ccitt.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_crc16_ccitt.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_minimal_cbor.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_minimal_cbor.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_rle.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_rle.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_varint.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/util/src/memfault_varint.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_battery.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_battery.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_connectivity.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_connectivity.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_reliability.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_reliability.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_metrics_serializer.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/metrics/src/memfault_metrics_serializer.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_regions_armv7.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_regions_armv7.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_sdk_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_sdk_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_storage_debug.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_storage_debug.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_coredump_utils.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_coredump_utils.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_aarch64.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_aarch64.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_arm.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_arm.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_armv7_a_r.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_armv7_a_r.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_fault_handling_riscv.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_fault_handling_riscv.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_components/memfault_stdlib_assert.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/panics/src/memfault_stdlib_assert.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/components/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/components/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_includes/ports/include</name>
\t\t<type>2</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/include</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos</name>
\t\t<type>2</type>
\t\t<locationURI>virtual:/virtual</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_core_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_core_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_freertos_ram_regions.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_freertos_ram_regions.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_metrics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_metrics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_panics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_panics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_sdk_metrics_freertos.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_sdk_metrics_freertos.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_sdk_metrics_thread.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_sdk_metrics_thread.c</locationURI>
\t</link>
\t<link>
\t\t<name>memfault_freertos/memfault_self_test_platform.c</name>
\t\t<type>1</type>
\t\t<locationURI>PARENT-3-PROJECT_LOC/ports/freertos/src/memfault_self_test_platform.c</locationURI>
\t</link>
\t</linkedResources>
</projectDescription>'''
}
