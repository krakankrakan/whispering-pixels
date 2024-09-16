package com.example.neuralnetleakvictim

import android.content.res.AssetManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.example.neuralnetleakvictim.ui.theme.NeuralNetLeakVictimTheme
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    external fun runComputeShader()
    external fun loadShaders(assetManager: AssetManager)

    private var job: Job? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            NeuralNetLeakVictimTheme {
                // A surface container using the 'background' color from the theme
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Greeting("Neural Net Leak Victim")
                }
            }
        }

        System.loadLibrary("neuralnetleakvictim")

        val assetManager: AssetManager = applicationContext.assets
        loadShaders(assetManager)

        // REMOVEME: Run only once
        runComputeShader()

        job = CoroutineScope(Dispatchers.Default).launch {
            while (isActive) {
                runComputeShader()
                delay(1)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        job?.cancel()
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    NeuralNetLeakVictimTheme {
        Greeting("Android")
    }
}