# Kernel do System Informer

- Otimiza a recuperação de informações do sistema
- Ativa a inspeção mais ampla para o sistema
- Informa sobre as atividades do sistema em tempo real
- Ajuda na remoção de malwares

## Segurança

[Procedimentos e Políticas de Segurança](../SECURITY.md)

Por conta da informção exposta através da ativação do driver, por desing, amplo
acesso ao sistema. Acesso é estritamente limitado aos ativadores verificados.
O acesso é restritamente baseado no estado do processo ativador. Isso envolve
assinatura, provilégios, e outros estados de confirmação. Se o cliente não quer
cumprir os requisitos do estado eles terão o acesso negado.

Qualquer binarie feita com a intenção de carregar algo para o System Informer
deve ter um `.sig` do par de chaves integrado no processo de desenvolvimento e driver.
Ou ser assinada pela Microsoft ou um fornecedor de software anti-vírus. O carregamento
de módulos não assinados vão restringir o acesso ao driver. Plugins third-party são
suportados, entretanto quando eles são carregados o acesso ao driver ficará restrito
se eles não forem assinados.

O driver rastreia clientes verificados, acessos restritos por outros autores
no sistema, e nega o acesso se o processo estiver adulterado. A intenção é para
desencorajar o exploit do cliente quando o driver estiver ativo. Se a adulteração
ou o exploit for detectado o cliente tem o acesso negado.  

## Desenvolvimento

Desenvolvedores podem gerar seu próprio par de chaves para usar no seu ambiente.

1. Executar `tools\CustomSignTool\bin\Release64\CustomSignTool.exe createkeypair kph.key public.key`
2. Copiar `kph.key` para `tools\CustomSignTool\resources`
3. Copiar os bytes de `public.key` para `KphpTrustedPublicKey` verifique em [verify.c](verify.c)

Uma vez que esses passos estiverem completos, componentes do System Informer
vão gerar um arquivo `.sig` próximo as binaries de saída. E o driver de
desenvolvimento vai usar uma chave específica realizando as etapas de verificação.
Qualquer plugin não feito através do processo regular de desenvolvimento deve
também ter seu próprio `.sig`.

Desenvolvedores talvez possam suprimir proteções e requisitos do estado alterando
`KPH_PROTECTION_SUPPRESSED` para `1` em [kphapi.h](../kphlib/include/kphapi.h).
Isso é necessário se você prentede usar o debugger no modo de usuário já que
as proteções e os requisitos do estado irão quebrar o debugger.
