<script>
import { Line } from "vue-chartjs";

export default {
  extends: Line,
  mounted() {
    this.renderChart(
      {
        labels: [
          "00:15",
          "00:30",
          "00:45",
          "01:00",
          "01:15",
          "01:30",
          "01:45",
          "02:00"
        ],
        datasets: [
          {
            label: "Temperatura",
            data: [50, 65, 68, 76, 78, 100, 100 ],
            backgroundColor: "transparent",
            borderColor: "rgba(1, 116, 188, 0.50)",
            pointBackgroundColor: "rgba(171, 71, 188, 1)"
          }
        ]
      },
      {
        responsive: true,
        maintainAspectRatio: false,
        title: {
          display: true,
          text: "Gráfico de Brassagem"
        }
      }
    );
  }
};
</script>