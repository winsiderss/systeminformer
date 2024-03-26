[![Build status](https://img.shields.io/github/workflow/status/winsiderss/systeminformer/continuous-integration?style=for-the-badge)](https://github.com/winsiderss/systeminformer/actions/workflows/msbuild.yml)
[![Build contributors](https://img.shields.io/github/contributors/winsiderss/systeminformer.svg?style=for-the-badge&color=blue)](https://github.com/winsiderss/systeminformer/graphs/contributors)
[![Licence](https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge&color=blue)](https://opensource.org/licenses/MIT)
[![Github stats](https://img.shields.io/github/downloads/winsiderss/systeminformer/total.svg?style=for-the-badge&color=red)](https://somsubhra.github.io/github-release-stats/?username=winsiderss&repository=systeminformer)
[![SourceForge stats](https://img.shields.io/sourceforge/dt/systeminformer.svg?style=for-the-badge&color=red)](https://sourceforge.net/projects/systeminformer/files/stats/timeline?dates=2008-10-01%20to%202020-09-01&period=monthly)

<img align="left" src="SystemInformer/resources/systeminformer.png" width="128" height="128"> 

## System Informer

Uma ferramenta gratuita, poderosa e com vários propósitos, que te ajuda a monitorar recursos do sistema, debugar softwares e detectar malwares. Feito para você por Winsider Seminars & Solutions, Inc.

[Site do Projeto](https://systeminformer.sourceforge.io/) - [Downloads do Projeto](https://systeminformer.sourceforge.io/downloads.php)

## Requisitos do Sistema

Windows 7 ou superior, 32-bit ou 64-bit.

## Funcionalidades

* Uma visão geral detalhada da atividade do sistema com realçamento (highlighting).
* Gráficos e estatísticas que permitem você rapidamente rastrear recursos gastando muito e processos infinitos.
* Não consegue editar ou deletar um arquivo? Descubra qual processo está usando aquele arquivo.
* Veja quais programas tem uma conexão com a internet ativa, e feche eles se necessário.
* Consiga informação em tempo real de acesso ao disco.
* Veja linhas de erro (stack traces) delathadas com modo kernel, WOW64 e suporte a .NET.
* Vá além de services.msc: Crie, edite e controle serviços.
* Pequeno, portátil e não requer instalação.
* 100% [Software Gratuito](https://www.gnu.org/philosophy/free-sw.en.html) ([MIT](https://opensource.org/licenses/MIT))


## Construindo o Projeto

Requer Visual Studio (2022 ou depois).

Executar `build_release.cmd` localizado no diretório `build` para compilar o projeto ou carregar o `SystemInformer.sln` e `Plugins.sln` se você preferir construir o projeto usando Visual Studio.

Você pode baixar a versão gratuita do [Visual Studio Community Edition](https://www.visualstudio.com/vs/community/) para construir o código fonte do System Informer.

Veja o [build readme_ptBR](./build/README_ptBR.md) para mais informações ou se você está tendo algum problema na hora de trabalhar no código.

## Melhorias/Bugs


Por favor use o [GitHub issue tracker](https://github.com/winsiderss/systeminformer/issues)
para reportar problemas ou segerir novas funcionalidades.


## Configurações

Se você está rodando o System Informer de um drive USB, talvez você queira
salvar as configurações dele no próprio USB. Para fazer isso, crie um
arquivo em branco chamado "SystemInformer.exe.settings.xml" no mesmo
diretório que o SystemInformer.exe. Você pode fazer isso com o Windows Explorer:

1. Tenha certeza que "Ocultar as extensões dos tipos de arquivos conhecidos" 
   esteja desmarcado em:
   Ferramentas > Opções da Pasta > Modo de Exibição.
2. Clique com o Botão Direito na pasta e escolha Novo > Documento de Texto.
3. Renomeie o arquivo para "SystemInformer.exe.settings.xml" (delete o ".txt").

## Plugins

Plugins podem ser configurados em Options > Plugins.

Se você tiver algum problema envolvendo plugins, tenha certeza que
eles estão atualizados.

Informações de Disco e Rede providas pelo plugin ExtendedTools estão
disponíveis apenas quando estiver rodando o System Informer com
permissões de administrador.
