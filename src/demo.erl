-module(demo).
-on_load(init/0).
-export([info/0, hello/1, goodbye/1, hello/2, echo/1, reverse/1]).

-define(nif_stub(),
	erlang:nif_error({nif_not_loaded,module,?MODULE,line,?LINE})).

init() ->
    Nif = filename:join(code:priv_dir(pynif), "demo"),
    erlang:load_nif(Nif, 0).

info() ->
    ?nif_stub().

hello(_Val) ->
    ?nif_stub().

goodbye(_Atom) ->
    ?nif_stub().

hello(_Val1, _Val2) ->
    ?nif_stub().

echo(_Term) ->
    ?nif_stub().
    
reverse(_List) ->
    ?nif_stub().
