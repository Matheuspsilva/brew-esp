<template>
<b-container class="bv-example-row">
  <div class="hello">
    <h1>{{ msg }}</h1>

          
          <div>
            <b-card-group deck>

              <b-card
                title="Temperatura"
                tag="article"
                style="max-width: 20rem;"
                class="mb-2"
              >
                <b-card-text>
                  <p id="temperaturec" name="temperaturec">%TEMPERATUREC%</p>
                  <sup class="units">&deg;C</sup>
                </b-card-text>

              </b-card>

              <b-card
                title="Receita"
                tag="article"
                style="max-width: 20rem;"
                class="mb-2"
              >
                <b-card-text>
                  American IPA
                </b-card-text>

                <b-form action="/get" method="get" >  
                    <div>
                        <b-button type="submit" variant="primary" name="brew_started" value="1">Iniciar</b-button>
                        <b-button>Editar</b-button>
                    </div>
                </b-form>
              </b-card>

              <b-card
                title="Configurações"
                tag="article"
                style="max-width: 20rem;"
                class="mb-2"
              >
                <b-card-text>
                  Outras configurações ...
                </b-card-text>

                <b-button href="#" variant="primary">Go somewhere</b-button>
              </b-card>

            </b-card-group>
          </div>


  </div>
</b-container>
</template>

<script>
export default {
  name: 'HelloWorld',
  props: {
    msg: String
  }
}
</script>

<script>
    setInterval(function ( ) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("temperaturec").innerHTML = this.responseText;
        }
      };
      xhttp.open("GET", "/temperaturec", true);
      xhttp.send();
    }, 10000) ;
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h3 {
  margin: 40px 0 0;
}
ul {
  list-style-type: none;
  padding: 0;
}
li {
  display: inline-block;
  margin: 0 10px;
}
a {
  color: #ffffff;
}
</style>
