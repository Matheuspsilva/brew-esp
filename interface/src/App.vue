<template>
  <div id="app">
    <img alt="Vue logo" src="./assets/logo.png">
    <HelloWorld msg="BrewSystem"/>
    <div class="container mt-3" style="background-color:#f1f1f1;">
      <div>
      <!-- <LineChart/> -->
      </div>
    </div>
  </div>
</template>

<script>
import HelloWorld from './components/HelloWorld.vue'
// import LineChart from "./components/LineChart.vue"

export default {
  name: 'App',
  components: {
    HelloWorld
    // ,LineChart
  }
}
</script>

<style>

html {
  height: 100%;
  /* Removing this one makes it look like it's working */
  background-color: #2c3e50;
}
.selector-for-some-widget {
  box-sizing: content-box;
}
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  /* color: #2c3e50; */
  margin-top: 60px;
  background-color: #2c3e50;
}
</style>
