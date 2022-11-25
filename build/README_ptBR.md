# Construindo o System Informer

A regra de ouro é rodar `build_release.cmd` depois de clonar o repositório. Você talez possa 
querer soluções diretamente no Visual Studio.

## Erros, Problemas, Gotchas

Essa sessão contém errs, problemas, e gotchas comuns enquanto está construindo o System Informer. 
Se você estiver tendo qualquer problema construindo então a informação aqui é provavel que
resolva seu problema.

### LINK : fatal error C1047 (thirdparty.lib)

TL;DR - run `build_thirdparty.cmd`

Esse erro ocorre pois o `thirdparty.lib` foi feito usando um compilador diferente daquele que
está no ambiente. Nós pré-construimos e checamos nas dependências third partys, para minimizar
o tempo de construção e simplificar o processo. Isso vem com o custo de que as vezes um programador
possa tentar desenvolver utilizando um compilador diferente. Para resolver isso no seu ambiente rode
`build_thirdparty.cmd`. Ou atualize o compliador MSVC no seu ambiente. Nós tentamos manter
o desenvolvimento e fazemos a checagem em um novo `thirdparty.lib` de maneira oportuna, 
é possivel que seu compilador esteja atulizado e nós não tenhamos feito a tarefa de 
manutenção para atulizar o `thirdparty.lib`.

### Plugins embutidos não estão carregnado com o driver kernel ativo

É possível carregar os plugins embutidos com o dirver ativo, mas existe uma mitigação de segurança
intencional para proteger o driver de qualquer abuso. Por favor, confira o
[kernel driver readme_ptBR](../KSystemInformer/README_ptBR.md) para mais informações. Talvez você
queira também as configurações avançadas para a negação no carregamento de imagem, mas o acesso ao
driver será restrito com plugins não assinados carregados.
